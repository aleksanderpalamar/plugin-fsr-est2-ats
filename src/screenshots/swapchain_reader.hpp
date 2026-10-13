#pragma once

#include "screenshots/interfaces.hpp"
#include "screenshots/d3d11_readback.hpp"

#include <dxgi.h>
#include <chrono>

namespace neuralfx {
class SwapchainScreenshotReader final : public ScreenshotReader {
public:
    void bind(IDXGISwapChain* swapchain) noexcept { swapchain_ = swapchain; }
    IDXGISwapChain* selected_swapchain() const noexcept { return selected_; }
    HWND selected_window() const noexcept { return window_; }
    bool prepare() override;
    std::optional<RgbImage> capture() override;
    void reset() noexcept {
        readback_.reset();
        swapchain_ = nullptr;
        selected_ = nullptr;
        window_ = nullptr;
        next_prepare_ = {};
    }

private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backbuffer();
    IDXGISwapChain* swapchain_ = nullptr;
    IDXGISwapChain* selected_ = nullptr;
    HWND window_ = nullptr;
    D3D11ScreenshotReadback readback_;
    std::chrono::steady_clock::time_point next_prepare_{};
};
}
