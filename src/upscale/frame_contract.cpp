#include "upscale/frame_contract.hpp"

#include <cmath>

namespace neuralfx::upscale {
namespace {
constexpr float max_jitter = 1.0f;

bool sizes_valid(const FrameContract& contract) {
    bool present = contract.render.width > 0 && contract.render.height > 0 && contract.output.width > 0 &&
        contract.output.height > 0;
    return present && contract.render.width <= contract.output.width && contract.render.height <= contract.output.height;
}

bool numbers_valid(const FrameContract& contract) {
    bool jitter = std::fabs(contract.jitter.x) <= max_jitter && std::fabs(contract.jitter.y) <= max_jitter;
    return jitter && std::isfinite(contract.frame_time_ms) && contract.frame_time_ms > 0.0f;
}
}

UpscaleStatus validate(const FrameContract& contract) noexcept {
    if (!sizes_valid(contract) || !numbers_valid(contract)) return UpscaleStatus::InvalidInput;
    if (contract.inputs != InputAvailability::Complete || contract.motion != MotionQuality::Valid)
        return UpscaleStatus::InvalidInput;
    return UpscaleStatus::Ready;
}

const char* status_name(UpscaleStatus status) noexcept {
    switch (status) {
    case UpscaleStatus::Ready: return "ready";
    case UpscaleStatus::Unsupported: return "unsupported";
    case UpscaleStatus::InvalidInput: return "invalid-input";
    case UpscaleStatus::UnavailableDependency: return "unavailable-dependency";
    case UpscaleStatus::DeviceLost: return "device-lost";
    case UpscaleStatus::BackendFailure: return "backend-failure";
    }
    return "unknown";
}
}
