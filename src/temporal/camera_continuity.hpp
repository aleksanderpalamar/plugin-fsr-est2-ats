#pragma once

#include "temporal/camera_state.hpp"

#include <cstdint>
#include <optional>

namespace neuralfx::temporal {
struct Position {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct CameraStep {
    std::uint64_t frame = 0;
    float distance_m = 0.0f;
    float angle_deg = 0.0f;
    Position from;
    Position to;
};

struct JumpLimits {
    float distance_m = 2.0f;
    float angle_deg = 20.0f;
};

bool is_jump(const CameraStep& step, JumpLimits limits = {}) noexcept;
std::optional<Position> camera_position(const Matrix4& view) noexcept;

class CameraContinuity {
public:
    std::optional<CameraStep> observe(const CameraObservation& observation) noexcept;
    void reset() noexcept { previous_.reset(); }

private:
    struct Placed {
        std::uint64_t frame = 0;
        Matrix4 view{};
        Matrix4 view_inverse{};
        Position position;
    };

    std::optional<Placed> previous_;
};
}
