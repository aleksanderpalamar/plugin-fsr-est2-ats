#include "screenshots/swapchain_selection.hpp"

#include <algorithm>

namespace neuralfx {
void ScreenshotSwapchainSelection::activated(const void* swapchain, const void* window) noexcept {
    if (!swapchain || !window) return;
    primary_ = swapchain;
    window_ = window;
}
void ScreenshotSwapchainSelection::created(const void* swapchain, const void* window) noexcept {
    if (!swapchain || !window) return;
    for (std::size_t index = 0; index < count_; ++index) {
        if (creations_[index].swapchain != swapchain) continue;
        creations_[index].window = window;
        return;
    }
    if (count_ == creations_.size()) {
        std::move(creations_.begin() + 1, creations_.end(), creations_.begin());
        --count_;
    }
    creations_[count_++] = {swapchain, window};
}

bool ScreenshotSwapchainSelection::presenting(const void* swapchain) noexcept {
    for (std::size_t index = 0; index < count_; ++index) {
        auto creation = creations_[index];
        if (creation.swapchain != swapchain) continue;
        std::move(creations_.begin() + index + 1, creations_.begin() + count_, creations_.begin() + index);
        --count_;
        bool replaced = primary_ && primary_ != swapchain && window_ == creation.window;
        if (replaced) primary_ = swapchain;
        return replaced;
    }
    return false;
}
void ScreenshotSwapchainSelection::reset() noexcept { primary_ = nullptr; window_ = nullptr; count_ = 0; }
}
