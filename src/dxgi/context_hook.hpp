#pragma once

#include <dxgi.h>

namespace neuralfx {
void hook_context(IDXGISwapChain* swapchain) noexcept;
void set_plugin_present(bool active) noexcept;
}
