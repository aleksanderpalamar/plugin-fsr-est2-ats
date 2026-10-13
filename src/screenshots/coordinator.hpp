#pragma once

#include "renderer/frame_status.hpp"
#include "screenshots/interfaces.hpp"

#include <atomic>

namespace neuralfx {
class ScreenshotCoordinator final : public ScreenshotEvents {
public:
    ScreenshotCoordinator(ScreenshotTransport& transport, ScreenshotObserver& observer)
        : transport_(transport), observer_(observer) {}
    void present(const void* swapchain, FrameStatus frame, ScreenshotReader& reader) noexcept;
    bool reset(const void* swapchain) noexcept;
    void requested() noexcept override;
    void ready(std::uint32_t handle, int result) noexcept override;

private:
    enum class State { Waiting, Active, Disabled };
    bool start(const void* swapchain, ScreenshotReader& reader);
    void process(const void* swapchain, FrameStatus frame, ScreenshotReader& reader);
    bool take_request() noexcept;
    void stop(ScreenshotEvent event) noexcept;
    void restore_requests(std::uint32_t count) noexcept;

    ScreenshotTransport& transport_;
    ScreenshotObserver& observer_;
    State state_ = State::Waiting;
    const void* active_swapchain_ = nullptr;
    std::atomic_uint32_t requests_{0};
    std::uint32_t in_flight_ = 0;
};
}
