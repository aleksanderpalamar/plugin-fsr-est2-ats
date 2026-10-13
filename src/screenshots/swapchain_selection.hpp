#pragma once

#include <array>
#include <cstddef>

namespace neuralfx {
class ScreenshotSwapchainSelection {
public:
    void activated(const void* swapchain, const void* window) noexcept;
    void created(const void* swapchain, const void* window) noexcept;
    bool presenting(const void* swapchain) noexcept;
    void reset() noexcept;

private:
    struct Creation { const void* swapchain; const void* window; };
    std::array<Creation, 16> creations_{};
    std::size_t count_ = 0;
    const void* primary_ = nullptr;
    const void* window_ = nullptr;
};
}
