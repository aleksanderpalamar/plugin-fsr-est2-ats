#pragma once

#include "renderer/frame_status.hpp"

#include <dxgi.h>

namespace neuralfx {
void present_steam_screenshot(IDXGISwapChain* swapchain, FrameStatus frame) noexcept;
void reset_steam_screenshots(IDXGISwapChain* swapchain) noexcept;
void notify_steam_swapchain(IDXGISwapChain* swapchain) noexcept;
}
