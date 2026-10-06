#pragma once

#include "config/config.hpp"

namespace neuralfx {
template <typename Swapchain>
bool resets_active_capture(Swapchain* active, Swapchain* resized) noexcept {
    return !resized || active == resized;
}

inline bool captures_depth(bool initialized, const Config& config) noexcept {
    return initialized && config.enabled && config.mode == TestMode::Raytracing;
}
}
