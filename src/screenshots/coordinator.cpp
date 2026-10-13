#include "screenshots/coordinator.hpp"

namespace neuralfx {
namespace {
constexpr std::uint32_t accepting = 1u << 31;
constexpr std::uint32_t count_mask = accepting - 1;
}

void ScreenshotCoordinator::present(const void* swapchain, FrameStatus frame,
    ScreenshotReader& reader) noexcept {
    try {
        process(swapchain, frame, reader);
    } catch (...) {
        stop(ScreenshotEvent::ReadFailed);
    }
}

bool ScreenshotCoordinator::start(const void* swapchain, ScreenshotReader& reader) {
    if (!transport_.available() || !reader.prepare()) return false;
    requests_.fetch_or(accepting);
    auto result = transport_.activate(*this);
    if (result == ScreenshotActivation::Active) {
        state_ = State::Active;
        active_swapchain_ = swapchain;
        observer_.report(ScreenshotEvent::Activated);
        return true;
    }
    requests_.store(0);
    if (result != ScreenshotActivation::Conflict) return false;
    active_swapchain_ = swapchain;
    state_ = State::Disabled;
    observer_.report(ScreenshotEvent::Conflict);
    return false;
}

void ScreenshotCoordinator::process(const void* swapchain, FrameStatus frame,
    ScreenshotReader& reader) {
    if (!swapchain || frame == FrameStatus::Skipped || state_ == State::Disabled) return;
    if (active_swapchain_ && active_swapchain_ != swapchain) return;
    if (frame == FrameStatus::Failed && state_ == State::Active) stop(ScreenshotEvent::Lost);
    if (frame == FrameStatus::Failed) return;
    if (state_ == State::Waiting && !start(swapchain, reader)) return;
    if (!transport_.active()) {
        stop(ScreenshotEvent::Lost);
        return;
    }
    if (!take_request()) return;
    auto image = reader.capture();
    if (!image) {
        stop(ScreenshotEvent::ReadFailed);
        return;
    }
    auto handle = transport_.write(*image);
    if (handle == 0) {
        stop(ScreenshotEvent::WriteFailed);
        return;
    }
    observer_.report(ScreenshotEvent::Submitted, handle);
}

bool ScreenshotCoordinator::reset(const void* swapchain) noexcept {
    if (swapchain && active_swapchain_ && swapchain != active_swapchain_) return false;
    requests_.fetch_and(count_mask);
    if (state_ == State::Active) transport_.deactivate();
    state_ = State::Waiting;
    active_swapchain_ = nullptr;
    return true;
}

void ScreenshotCoordinator::stop(ScreenshotEvent event) noexcept {
    requests_.store(0);
    if (state_ == State::Active) transport_.deactivate();
    state_ = State::Disabled;
    observer_.report(event);
}

void ScreenshotCoordinator::requested() noexcept {
    auto current = requests_.load();
    while ((current & accepting) && (current & count_mask) < 32) {
        if (requests_.compare_exchange_weak(current, current + 1)) return;
    }
}

bool ScreenshotCoordinator::take_request() noexcept {
    auto current = requests_.load();
    while ((current & accepting) && (current & count_mask) != 0) {
        if (requests_.compare_exchange_weak(current, current - 1)) return true;
    }
    return false;
}

void ScreenshotCoordinator::ready(std::uint32_t handle, int result) noexcept {
    observer_.report(handle && result == 1 ? ScreenshotEvent::Ready : ScreenshotEvent::ReadyFailed,
        handle, result);
}
}
