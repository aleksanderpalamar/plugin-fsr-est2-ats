#pragma once

#include "temporal/projection_jitter.hpp"

#include <optional>

namespace neuralfx::temporal {
struct CameraMatrices {
    Matrix4 view{};
    Matrix4 view_projection{};
};

struct CameraState {
    Jitter jitter;
    Matrix4 projection{};
    Matrix4 view_projection{};
};

std::optional<CameraState> camera_state(const CameraMatrices& matrices, RenderSize size) noexcept;
}
