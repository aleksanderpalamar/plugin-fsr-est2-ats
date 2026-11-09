#pragma once

#include "temporal/projection_jitter.hpp"

#include <cstdint>
#include <optional>
#include <variant>

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

struct CameraRejection {
    CameraFault fault = CameraFault::SingularView;
    Matrix4 projection{};
};

struct CameraObservation {
    std::uint64_t frame = 0;
    CameraMatrices matrices;
};

std::variant<CameraState, CameraRejection> evaluate_camera(const CameraMatrices& matrices, RenderSize size) noexcept;
std::optional<CameraState> camera_state(const CameraMatrices& matrices, RenderSize size) noexcept;
}
