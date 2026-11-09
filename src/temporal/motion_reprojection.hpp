#pragma once

#include "temporal/camera_state.hpp"

#include <optional>

namespace neuralfx::temporal {
struct MotionConstants {
    Matrix4 reprojection{};
    Jitter jitter;
    RenderSize size;
};

struct PixelMotion {
    float x = 0.0f;
    float y = 0.0f;
};

std::optional<MotionConstants> motion_constants(const CameraState& previous, const CameraState& current,
    RenderSize size) noexcept;
std::optional<PixelMotion> camera_motion(const MotionConstants& constants, float x, float y, float depth) noexcept;
}
