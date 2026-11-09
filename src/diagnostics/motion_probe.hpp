#pragma once

#include <dxgi.h>

namespace neuralfx::diagnostics {
void present_motion_probe(IDXGISwapChain* swapchain) noexcept;
void motion_probe_frame_end(IDXGISwapChain* swapchain) noexcept;
}
