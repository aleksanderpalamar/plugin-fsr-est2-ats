#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <optional>
#include <wrl/client.h>

namespace neuralfx {
struct SwapchainAccess {
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backbuffer;
};

std::optional<SwapchainAccess> access_swapchain(IDXGISwapChain* swapchain) noexcept;
}
