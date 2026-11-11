#pragma once

#include "temporal/projection_jitter.hpp"

namespace neuralfx::upscale {
enum class UpscaleStatus { Ready, Unsupported, InvalidInput, UnavailableDependency, DeviceLost, BackendFailure };
enum class InputAvailability { Complete, NoColor, NoDepth, NoMotion };
enum class MotionQuality { Valid, Invalid };

struct CameraParameters {
    float near_plane = 0.1f;
    float far_plane = 1600.0f;
    float vertical_fov = 0.9f;
};

struct FrameContract {
    temporal::RenderSize render;
    temporal::RenderSize output;
    temporal::Jitter jitter;
    float frame_time_ms = 0.0f;
    InputAvailability inputs = InputAvailability::Complete;
    MotionQuality motion = MotionQuality::Valid;
    CameraParameters camera;
    bool reset = false;
};

enum class ContractFault { None, Sizes, Numbers, Camera, Inputs };

bool can_upscale(temporal::RenderSize render, temporal::RenderSize output) noexcept;
ContractFault diagnose(const FrameContract& contract) noexcept;
UpscaleStatus validate(const FrameContract& contract) noexcept;
const char* fault_name(ContractFault fault) noexcept;
const char* status_name(UpscaleStatus status) noexcept;
}
