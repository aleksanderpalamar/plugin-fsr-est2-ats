#include "dxgi/swapchain_access.hpp"

namespace neuralfx {
std::optional<SwapchainAccess> access_swapchain(IDXGISwapChain* swapchain) noexcept {
    SwapchainAccess result;
    if (!swapchain || FAILED(swapchain->GetDevice(IID_PPV_ARGS(&result.device)))) return std::nullopt;
    result.device->GetImmediateContext(&result.context);
    if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&result.backbuffer))) || !result.context) return std::nullopt;
    return result;
}
}
