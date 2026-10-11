#include "dxgi/dxgi_proxy.hpp"
#include "dxgi/system_dxgi.hpp"
#include "dxgi/swapchain_hook.hpp"
#include "common/logger.hpp"

#include <dxgi1_2.h>
#include <bit>
#include <string>
#include <type_traits>

namespace {
template <typename FactoryFunction>
HRESULT create_factory(const char* name, UINT flags, REFIID iid, void** output) noexcept {
    if (!output) return E_POINTER;
    *output = nullptr;
    auto function = std::bit_cast<FactoryFunction>(ResolveDxgiExport(name));
    if (!function) {
        DWORD error = GetLastError();
        return error == ERROR_SUCCESS ? DXGI_ERROR_UNSUPPORTED : HRESULT_FROM_WIN32(error);
    }
    HRESULT result;
    if constexpr (std::is_invocable_r_v<HRESULT, FactoryFunction, UINT, REFIID, void**>) {
        result = function(flags, iid, output);
    } else {
        result = function(iid, output);
    }
    if (FAILED(result))
        neuralfx::log("System DXGI factory failed: name=" + std::string(name)
            + " hresult=" + std::to_string(static_cast<unsigned long>(result)));
    if (SUCCEEDED(result) && *output) neuralfx::hook_factory(static_cast<IUnknown*>(*output));
    return result;
}

void* factory_hook(void* address) noexcept {
    if (address == find_system_dxgi_export("CreateDXGIFactory"))
        return std::bit_cast<void*>(&CreateDXGIFactory);
    if (address == find_system_dxgi_export("CreateDXGIFactory1"))
        return std::bit_cast<void*>(&CreateDXGIFactory1);
    if (address == find_system_dxgi_export("CreateDXGIFactory2"))
        return std::bit_cast<void*>(&CreateDXGIFactory2);
    return address;
}
}

extern "C" void* __cdecl ResolveDxgiExport(const char* name) noexcept {
    return resolve_system_dxgi_export(name);
}

extern "C" void* __cdecl ResolveDxgiOrdinal(unsigned ordinal) noexcept {
    void* address = resolve_system_dxgi_ordinal(ordinal);
    if (!address) return nullptr;
    return factory_hook(address);
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
