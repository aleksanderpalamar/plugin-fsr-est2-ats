#pragma once

#include <unknwn.h>
#include <dxgi.h>

namespace neuralfx {
void hook_factory(IUnknown* factory) noexcept;
void hook_swapchain(IDXGISwapChain* swapchain) noexcept;
}
