#include "dxgi/swapchain_hook.hpp"
#include "dxgi/context_hook.hpp"
#include "dxgi/vtable_patch.hpp"
#include "renderer/renderer.hpp"
#include "common/logger.hpp"

#include <windows.h>
#include <dxgi1_2.h>
#include <mutex>
#include <wrl/client.h>

namespace neuralfx {
namespace {
using CreateSwapChain = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
using CreateForHwnd = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);
using CreateForCore = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**);
using CreateForComposition = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**);
using Present = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using Resize = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

std::mutex hook_mutex;
CreateSwapChain original_create = nullptr;
CreateForHwnd original_hwnd = nullptr;
CreateForCore original_core = nullptr;
CreateForComposition original_composition = nullptr;
Present original_present = nullptr;
Resize original_resize = nullptr;

HRESULT STDMETHODCALLTYPE on_create(IDXGIFactory* factory, IUnknown* device, DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** output) noexcept {
    if (!original_create) return E_FAIL;
    HRESULT result = original_create(factory, device, desc, output);
    if (SUCCEEDED(result) && output && *output) hook_swapchain(*output);
    return result;
}

HRESULT STDMETHODCALLTYPE on_hwnd(IDXGIFactory2* factory, IUnknown* device, HWND window, const DXGI_SWAP_CHAIN_DESC1* desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen, IDXGIOutput* output, IDXGISwapChain1** swapchain) noexcept {
    if (!original_hwnd) return E_FAIL;
    HRESULT result = original_hwnd(factory, device, window, desc, fullscreen, output, swapchain);
    if (SUCCEEDED(result) && swapchain && *swapchain) hook_swapchain(*swapchain);
    return result;
}

HRESULT STDMETHODCALLTYPE on_core(IDXGIFactory2* factory, IUnknown* device, IUnknown* window, const DXGI_SWAP_CHAIN_DESC1* desc, IDXGIOutput* output, IDXGISwapChain1** swapchain) noexcept {
    if (!original_core) return E_FAIL;
    HRESULT result = original_core(factory, device, window, desc, output, swapchain);
    if (SUCCEEDED(result) && swapchain && *swapchain) hook_swapchain(*swapchain);
    return result;
}

HRESULT STDMETHODCALLTYPE on_composition(IDXGIFactory2* factory, IUnknown* device, const DXGI_SWAP_CHAIN_DESC1* desc, IDXGIOutput* output, IDXGISwapChain1** swapchain) noexcept {
    if (!original_composition) return E_FAIL;
    HRESULT result = original_composition(factory, device, desc, output, swapchain);
    if (SUCCEEDED(result) && swapchain && *swapchain) hook_swapchain(*swapchain);
    return result;
}

HRESULT STDMETHODCALLTYPE on_present(IDXGISwapChain* swapchain, UINT interval, UINT flags) noexcept {
    if (!original_present) return E_FAIL;
    static thread_local bool in_hook = false;
    if (in_hook || (flags & DXGI_PRESENT_TEST)) return original_present(swapchain, interval, flags);
    in_hook = true;
    set_plugin_present(true);
    try {
        Renderer::instance().present(swapchain);
    } catch (...) {
        log("Unexpected exception in Present hook");
    }
    set_plugin_present(false);
    in_hook = false;
    return original_present(swapchain, interval, flags);
}

HRESULT STDMETHODCALLTYPE on_resize(IDXGISwapChain* swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags) noexcept {
    if (!original_resize) return E_FAIL;
    try {
        Renderer::instance().reset(swapchain);
    } catch (...) {
        log("Unexpected exception in ResizeBuffers hook");
    }
    HRESULT result = original_resize(swapchain, count, width, height, format, flags);
    if (SUCCEEDED(result)) hook_context(swapchain);
    return result;
}
}

void hook_factory(IUnknown* factory) noexcept {
    if (!factory) return;
    std::lock_guard lock(hook_mutex);
    Microsoft::WRL::ComPtr<IDXGIFactory> base;
    if (FAILED(factory->QueryInterface(IID_PPV_ARGS(&base)))) return;
    patch_vtable<CreateSwapChain>(base.Get(), 10, &on_create, original_create);
    Microsoft::WRL::ComPtr<IDXGIFactory2> factory2;
    if (FAILED(factory->QueryInterface(IID_PPV_ARGS(&factory2)))) return;
    patch_vtable<CreateForHwnd>(factory2.Get(), 15, &on_hwnd, original_hwnd);
    patch_vtable<CreateForCore>(factory2.Get(), 16, &on_core, original_core);
    patch_vtable<CreateForComposition>(factory2.Get(), 24, &on_composition, original_composition);
}

void hook_swapchain(IDXGISwapChain* swapchain) noexcept {
    if (!swapchain) return;
    std::lock_guard lock(hook_mutex);
    bool present_hooked = patch_vtable<Present>(swapchain, 8, &on_present, original_present);
    bool resize_hooked = patch_vtable<Resize>(swapchain, 13, &on_resize, original_resize);
    if (present_hooked && resize_hooked) log("Swapchain Present and ResizeBuffers hooked");
    hook_context(swapchain);
}
}
