#include "screenshots/steam_interception.hpp"
#include "screenshots/steam_imports.hpp"
#include "common/logger.hpp"

#include <bit>
#include <cstdio>

namespace neuralfx {
namespace {
constinit SteamCallbackRegistration original_api{};
constinit bool installed = false;
constinit DWORD install_error = ERROR_SUCCESS;
constinit std::atomic<DWORD> dispatch_error{ERROR_SUCCESS};

SteamCallbackRelays& relays() {
    static auto* instance = new SteamCallbackRelays(original_api);
    return *instance;
}

void register_callback(SteamCallback* callback, int id) noexcept {
    try {
        relays().register_callback(callback, id);
    } catch (...) {
        dispatch_error.store(ERROR_UNHANDLED_EXCEPTION);
        original_api.add(callback, id);
    }
}

void unregister_callback(SteamCallback* callback) noexcept {
    try {
        relays().unregister_callback(callback);
    } catch (...) {
        dispatch_error.store(ERROR_UNHANDLED_EXCEPTION);
        original_api.remove(callback);
    }
}

std::span<std::byte> executable_image() noexcept {
    auto* image = reinterpret_cast<std::byte*>(GetModuleHandleW(nullptr));
    if (!image) return {};
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0) return {};
    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return {};
    return {image, nt->OptionalHeader.SizeOfImage};
}

bool restore(void** slot, DWORD protection) noexcept {
    DWORD unused = 0;
    if (VirtualProtect(slot, sizeof(void*), protection, &unused)) return true;
    install_error = GetLastError();
    return false;
}

bool patch(void** registration, void** unregistration) noexcept {
    DWORD registration_protection = 0;
    DWORD unregistration_protection = 0;
    if (!VirtualProtect(registration, sizeof(void*), PAGE_READWRITE, &registration_protection)) {
        install_error = GetLastError();
        return false;
    }
    if (!VirtualProtect(unregistration, sizeof(void*), PAGE_READWRITE, &unregistration_protection)) {
        install_error = GetLastError();
        restore(registration, registration_protection);
        return false;
    }
    original_api.add = std::bit_cast<decltype(original_api.add)>(*registration);
    original_api.remove = std::bit_cast<decltype(original_api.remove)>(*unregistration);
    InterlockedExchangePointer(registration, std::bit_cast<void*>(&register_callback));
    InterlockedExchangePointer(unregistration, std::bit_cast<void*>(&unregister_callback));
    bool unregistration_restored = restore(unregistration, unregistration_protection);
    bool registration_restored = restore(registration, registration_protection);
    return unregistration_restored && registration_restored;
}
}

void install_steam_callback_interception() noexcept {
    if (installed) return;
    auto image = executable_image();
    auto imports = steam_callback_imports(image);
    if (!imports) {
        install_error = ERROR_PROC_NOT_FOUND;
        return;
    }
    auto** registration = reinterpret_cast<void**>(image.data() + imports->registration);
    auto** unregistration = reinterpret_cast<void**>(image.data() + imports->unregistration);
    if (!*registration || !*unregistration) {
        install_error = ERROR_INVALID_ADDRESS;
        return;
    }
    installed = patch(registration, unregistration);
}

SteamScreenshotRelaySource* game_steam_screenshot_relays() {
    return installed && dispatch_error.load() == ERROR_SUCCESS ? &relays() : nullptr;
}

void log_steam_callback_interception() noexcept {
    char message[160]{};
    std::snprintf(message, sizeof(message), "Steam screenshot callback interception: installed=%d win32=%lu dispatch=%lu",
        installed, static_cast<unsigned long>(install_error), static_cast<unsigned long>(dispatch_error.load()));
    log(message);
}
}
