#pragma once

#include <dxgi.h>

namespace neuralfx::upscale {
void upscale_frame_end(IDXGISwapChain* swapchain) noexcept;
void upscale_present(IDXGISwapChain* swapchain) noexcept;
bool upscale_active() noexcept;
}
