#include "temporal/camera_continuity.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace neuralfx::temporal {
namespace {
constexpr double radians_to_degrees = 57.29577951308232;

Position position_of(const Matrix4& view_inverse) {
    return {view_inverse[3], view_inverse[7], view_inverse[11]};
}

float distance(Position from, Position to) {
    double x = static_cast<double>(to.x) - from.x;
    double y = static_cast<double>(to.y) - from.y;
    double z = static_cast<double>(to.z) - from.z;
    return static_cast<float>(std::sqrt(x * x + y * y + z * z));
}

float turn(const Matrix4& previous_view, const Matrix4& current_inverse) {
    auto relative = multiply(previous_view, current_inverse);
    double cosine = (static_cast<double>(relative[0]) + relative[5] + relative[10] - 1.0) * 0.5;
    return static_cast<float>(std::acos(std::clamp(cosine, -1.0, 1.0)) * radians_to_degrees);
}
}

bool is_jump(const CameraStep& step, JumpLimits limits) noexcept {
    return step.distance_m > limits.distance_m || step.angle_deg > limits.angle_deg;
}

std::optional<Position> camera_position(const Matrix4& view) noexcept {
    auto view_inverse = inverse(view);
    if (!view_inverse) return std::nullopt;
    return position_of(*view_inverse);
}

std::optional<CameraStep> CameraContinuity::observe(const CameraObservation& observation) noexcept {
    auto view_inverse = inverse(observation.matrices.view);
    if (!view_inverse) {
        previous_.reset();
        return std::nullopt;
    }
    Placed current{observation.frame, observation.matrices.view, *view_inverse, position_of(*view_inverse)};
    auto previous = std::exchange(previous_, current);
    if (!previous || previous->frame + 1 != current.frame) return std::nullopt;
    return CameraStep{current.frame, distance(previous->position, current.position),
        turn(previous->view, current.view_inverse), previous->position, current.position};
}
}
