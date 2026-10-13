#include "screenshots/steam_loader.hpp"
#include "screenshots/steam_imports.hpp"
#include "screenshots/steam_interception.hpp"
#include "common/logger.hpp"

#include <psapi.h>
#include <bit>
#include <cstdio>

namespace neuralfx {
namespace {
template <typename Function>
Function resolve(HMODULE module, const char* name) noexcept {
    auto address = GetProcAddress(module, name);
    if (address) return std::bit_cast<Function>(address);
    char message[160]{};
    std::snprintf(message, sizeof(message), "Steam screenshot export unavailable: %s win32=%lu",
        name, static_cast<unsigned long>(GetLastError()));
    log(message);
    return nullptr;
}

bool standard_dispatch() noexcept {
    MODULEINFO info{};
    if (!K32GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(nullptr), &info, sizeof(info)))
        return false;
    return uses_standard_steam_dispatch({static_cast<const std::byte*>(info.lpBaseOfDll), info.SizeOfImage});
}

SteamScreenshotFunctions bind(HMODULE module) noexcept {
    SteamScreenshotFunctions functions;
    functions.user = resolve<decltype(functions.user)>(module, "SteamAPI_GetHSteamUser");
    functions.pipe = resolve<decltype(functions.pipe)>(module, "SteamAPI_GetHSteamPipe");
    functions.screenshots = resolve<decltype(functions.screenshots)>(module, "SteamAPI_SteamScreenshots_v003");
    functions.register_callback = resolve<decltype(functions.register_callback)>(module, "SteamAPI_RegisterCallback");
    functions.unregister_callback = resolve<decltype(functions.unregister_callback)>(module, "SteamAPI_UnregisterCallback");
    functions.hook = resolve<decltype(functions.hook)>(module, "SteamAPI_ISteamScreenshots_HookScreenshots");
    functions.hooked = resolve<decltype(functions.hooked)>(module, "SteamAPI_ISteamScreenshots_IsScreenshotsHooked");
    functions.write = resolve<decltype(functions.write)>(module, "SteamAPI_ISteamScreenshots_WriteScreenshot");
    functions.standard_dispatch = standard_dispatch();
    return functions;
}

bool complete(const SteamScreenshotFunctions& api) noexcept {
    return api.standard_dispatch && api.user && api.pipe && api.screenshots &&
        api.register_callback && api.unregister_callback && api.hook && api.hooked && api.write;
}
}

void SteamScreenshotLoader::poll() noexcept {
    if (state_ == State::Ready || state_ == State::Unsupported) return;
    auto now = std::chrono::steady_clock::now();
    if (now < next_poll_) return;
    next_poll_ = now + std::chrono::seconds(1);
    HMODULE module = nullptr;
    bool loaded = GetModuleHandleExW(0, L"steam_api64.dll", &module) != FALSE;
    if (!loaded && state_ == State::Waiting) log("Steam screenshots unavailable: steam_api64.dll is not loaded");
    if (!loaded) {
        state_ = State::Missing;
        return;
    }
    auto functions = bind(module);
    if (!complete(functions)) {
        log("Steam screenshots unavailable: incomplete API or unsupported game callback dispatch");
        FreeLibrary(module);
        state_ = State::Unsupported;
        return;
    }
    module_ = module;
    functions_ = functions;
    state_ = State::Ready;
    log_steam_callback_interception();
    log("Steam screenshots API resolved; using the game's SteamAPI_RunCallbacks dispatcher");
}
}
