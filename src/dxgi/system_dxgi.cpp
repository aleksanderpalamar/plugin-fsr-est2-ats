#include "dxgi/system_dxgi.hpp"
#include "dxgi/system_dxgi_provider.hpp"
#include "common/logger.hpp"

#include <bit>
#include <string>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {
SystemDxgi load_system_dxgi() noexcept {
    wchar_t path[MAX_PATH]{};
    UINT length = GetSystemDirectoryW(path, MAX_PATH);
    if (length == 0) return {nullptr, GetLastError(), ERROR_SUCCESS};
    if (length >= MAX_PATH - 9) return {nullptr, ERROR_INSUFFICIENT_BUFFER, ERROR_SUCCESS};
    if (wcscat_s(path, L"\\dxgi.dll") != 0)
        return {nullptr, ERROR_INVALID_NAME, ERROR_SUCCESS};

    HMODULE module = LoadLibraryExW(path, nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (module) return {module, ERROR_SUCCESS, ERROR_SUCCESS};
    DWORD search_error = GetLastError();
    module = LoadLibraryW(path);
    if (!module) return {nullptr, search_error, GetLastError()};
    return {module, search_error, ERROR_SUCCESS};
}

SystemDxgi load_checked_system_dxgi() noexcept {
    SystemDxgi loaded = load_system_dxgi();
    if (!loaded.module) {
        neuralfx::log("System DXGI load failed: search_win32="
            + std::to_string(loaded.search_error) + " fallback_win32="
            + std::to_string(loaded.fallback_error));
        return loaded;
    }
    if (loaded.module != reinterpret_cast<HMODULE>(&__ImageBase)) return loaded;
    neuralfx::log("System DXGI load returned the proxy module");
    return {nullptr, ERROR_INVALID_DATA, ERROR_SUCCESS};
}

SystemDxgi system_dxgi() noexcept {
    static SystemDxgiProvider provider(load_checked_system_dxgi);
    return provider.get();
}

void* find_export(const char* name) noexcept {
    const auto library = system_dxgi();
    if (!library.module) {
        SetLastError(library.fallback_error == ERROR_SUCCESS
            ? library.search_error : library.fallback_error);
        return nullptr;
    }
    if (!name) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return nullptr;
    }
    return std::bit_cast<void*>(GetProcAddress(library.module, name));
}
}

void* find_system_dxgi_export(const char* name) noexcept {
    return find_export(name);
}

void* resolve_system_dxgi_export(const char* name) noexcept {
    void* address = find_export(name);
    if (address) return address;
    DWORD error = GetLastError();
    neuralfx::log("System DXGI export unavailable: name="
        + std::string(name ? name : "<null>") + " win32=" + std::to_string(error));
    SetLastError(error);
    return nullptr;
}

void* resolve_system_dxgi_ordinal(unsigned ordinal) noexcept {
    const auto library = system_dxgi();
    if (!library.module || ordinal == 0 || ordinal > 0xffff) {
        DWORD error = library.module ? ERROR_INVALID_PARAMETER
            : (library.fallback_error == ERROR_SUCCESS ? library.search_error : library.fallback_error);
        SetLastError(error);
        neuralfx::log("System DXGI ordinal unavailable: ordinal="
            + std::to_string(ordinal) + " win32=" + std::to_string(error));
        return nullptr;
    }
    void* address = std::bit_cast<void*>(GetProcAddress(
        library.module, MAKEINTRESOURCEA(static_cast<WORD>(ordinal))));
    if (address) return address;
    DWORD error = GetLastError();
    neuralfx::log("System DXGI ordinal unavailable: ordinal="
        + std::to_string(ordinal) + " win32=" + std::to_string(error));
    SetLastError(error);
    return nullptr;
}
