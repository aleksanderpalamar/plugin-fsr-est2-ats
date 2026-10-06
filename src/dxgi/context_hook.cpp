#include "dxgi/context_hook.hpp"
#include "dxgi/depth_capture.hpp"
#include "dxgi/vtable_patch.hpp"
#include "common/logger.hpp"

#include <d3d11.h>
#include <mutex>
#include <wrl/client.h>

namespace neuralfx {
namespace {
using SetTargets = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT,
    ID3D11RenderTargetView* const*, ID3D11DepthStencilView*);

std::mutex hook_mutex;
SetTargets original_set_targets = nullptr;
thread_local bool plugin_present = false;

void STDMETHODCALLTYPE on_set_targets(ID3D11DeviceContext* context, UINT count,
    ID3D11RenderTargetView* const* targets, ID3D11DepthStencilView* depth) noexcept {
    if (!original_set_targets) return;
    if (plugin_present || !DepthCapture::instance().active()) {
        original_set_targets(context, count, targets, depth);
        return;
    }
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> previous;
    context->OMGetRenderTargets(0, nullptr, &previous);
    original_set_targets(context, count, targets, depth);
    if (!previous || previous.Get() == depth) return;
    DepthCapture::instance().capture(context, previous.Get());
}
}

void set_plugin_present(bool active) noexcept {
    plugin_present = active;
}

void hook_context(IDXGISwapChain* swapchain) noexcept {
    if (!swapchain) return;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    if (FAILED(swapchain->GetDevice(IID_PPV_ARGS(&device)))) return;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(&context);
    if (!context) return;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backbuffer;
    if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)))) return;
    D3D11_TEXTURE2D_DESC description{};
    backbuffer->GetDesc(&description);
    DepthCapture::instance().configure(device.Get(), description.Width, description.Height);
    std::lock_guard lock(hook_mutex);
    if (patch_vtable<SetTargets>(context.Get(), 33, &on_set_targets, original_set_targets))
        log("D3D11 depth target hook ready");
}
}
