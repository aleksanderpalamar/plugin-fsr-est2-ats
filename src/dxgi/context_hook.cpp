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
using SetTargetsAndUavs = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT,
    ID3D11RenderTargetView* const*, ID3D11DepthStencilView*, UINT, UINT,
    ID3D11UnorderedAccessView* const*, const UINT*);

std::mutex hook_mutex;
SetTargets original_set_targets = nullptr;
SetTargetsAndUavs original_set_targets_and_uavs = nullptr;
thread_local bool plugin_present = false;

Microsoft::WRL::ComPtr<ID3D11DepthStencilView> bound_depth(ID3D11DeviceContext* context) noexcept {
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth;
    context->OMGetRenderTargets(0, nullptr, &depth);
    return depth;
}

template <typename Forward>
void forward_depth_binding(ID3D11DeviceContext* context, Forward&& forward) noexcept {
    if (plugin_present || !DepthCapture::instance().active()) {
        forward();
        return;
    }
    auto previous = bound_depth(context);
    forward();
    if (!previous || previous.Get() == bound_depth(context).Get()) return;
    DepthCapture::instance().capture(context, previous.Get());
}

void STDMETHODCALLTYPE on_set_targets(ID3D11DeviceContext* context, UINT count,
    ID3D11RenderTargetView* const* targets, ID3D11DepthStencilView* depth) noexcept {
    if (!original_set_targets) return;
    forward_depth_binding(context, [&] {
        original_set_targets(context, count, targets, depth);
    });
}

void STDMETHODCALLTYPE on_set_targets_and_uavs(ID3D11DeviceContext* context, UINT count,
    ID3D11RenderTargetView* const* targets, ID3D11DepthStencilView* depth,
    UINT start, UINT uav_count, ID3D11UnorderedAccessView* const* uavs,
    const UINT* counts) noexcept {
    if (!original_set_targets_and_uavs) return;
    forward_depth_binding(context, [&] {
        original_set_targets_and_uavs(context, count, targets, depth,
            start, uav_count, uavs, counts);
    });
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
    std::lock_guard lock(hook_mutex);
    bool targets_hooked = patch_vtable<SetTargets>(context.Get(), 33,
        &on_set_targets, original_set_targets);
    bool uavs_hooked = patch_vtable<SetTargetsAndUavs>(context.Get(), 34,
        &on_set_targets_and_uavs, original_set_targets_and_uavs);
    if (targets_hooked && uavs_hooked)
        log("D3D11 depth target hook ready");
}
}
