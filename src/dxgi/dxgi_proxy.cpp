#include "dxgi/dxgi_proxy.hpp"
#include "dxgi/swapchain_hook.hpp"
#include "common/logger.hpp"

#include <dxgi1_2.h>
#include <bit>
#include <cwchar>
#include <type_traits>

namespace {
HMODULE real_dxgi() noexcept {
    static HMODULE module = []() noexcept {
        wchar_t path[MAX_PATH]{};
        UINT length = GetSystemDirectoryW(path, MAX_PATH);
        if (length == 0 || length >= MAX_PATH - 9) return static_cast<HMODULE>(nullptr);
        if (wcscat_s(path, L"\\dxgi.dll") != 0) return static_cast<HMODULE>(nullptr);
        return LoadLibraryW(path);
    }();
    return module;
}

template <typename FactoryFunction>
HRESULT create_factory(const char* name, UINT flags, REFIID iid, void** output) noexcept {
    if (!output) return E_POINTER;
    *output = nullptr;
    auto function = std::bit_cast<FactoryFunction>(ResolveDxgiExport(name));
    if (!function) {
        neuralfx::log("System DXGI export unavailable");
        return DXGI_ERROR_UNSUPPORTED;
    }
    HRESULT result;
    if constexpr (std::is_invocable_r_v<HRESULT, FactoryFunction, UINT, REFIID, void**>) {
        result = function(flags, iid, output);
    } else {
        result = function(iid, output);
    }
    if (SUCCEEDED(result) && *output) neuralfx::hook_factory(static_cast<IUnknown*>(*output));
    return result;
}
}

extern "C" void* __cdecl ResolveDxgiExport(const char* name) noexcept {
    HMODULE module = real_dxgi();
    if (!module || !name) return nullptr;
    return std::bit_cast<void*>(GetProcAddress(module, name));
}

extern "C" HRESULT WINAPI CreateDXGIFactory(REFIID iid, void** output) {
    using Function = HRESULT(WINAPI*)(REFIID, void**);
    return create_factory<Function>("CreateDXGIFactory", 0, iid, output);
}

extern "C" HRESULT WINAPI CreateDXGIFactory1(REFIID iid, void** output) {
    using Function = HRESULT(WINAPI*)(REFIID, void**);
    return create_factory<Function>("CreateDXGIFactory1", 0, iid, output);
}

extern "C" HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID iid, void** output) {
    using Function = HRESULT(WINAPI*)(UINT, REFIID, void**);
    return create_factory<Function>("CreateDXGIFactory2", flags, iid, output);
}
