#pragma once

#include "temporal/projection_jitter.hpp"

namespace neuralfx::upscale {
enum class UpscaleStatus { Ready, Unsupported, InvalidInput, UnavailableDependency, DeviceLost, BackendFailure };
enum class InputAvailability { Complete, NoColor, NoDepth, NoMotion };
enum class MotionQuality { Valid, Invalid };

struct FrameContract {
    temporal::RenderSize render;
    temporal::RenderSize output;
    temporal::Jitter jitter;
    float frame_time_ms = 0.0f;
    InputAvailability inputs = InputAvailability::Complete;
    MotionQuality motion = MotionQuality::Valid;
    bool reset = false;
};

UpscaleStatus validate(const FrameContract& contract) noexcept;
const char* status_name(UpscaleStatus status) noexcept;
}
