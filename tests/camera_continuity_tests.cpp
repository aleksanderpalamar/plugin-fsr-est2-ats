#include "temporal/camera_continuity.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx::temporal;

namespace {
constexpr float degrees = 3.14159265f / 180.0f;

Matrix4 camera_at(float x, float z, float yaw) {
    float c = std::cos(yaw * degrees);
    float s = std::sin(yaw * degrees);
    Matrix4 rotation{c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    Matrix4 translation{1.0f, 0.0f, 0.0f, -x, 0.0f, 1.0f, 0.0f, -1.5f, 0.0f, 0.0f, 1.0f, -z, 0.0f, 0.0f, 0.0f, 1.0f};
    return multiply(rotation, translation);
}

CameraObservation seen(std::uint64_t frame, const Matrix4& view) {
    return {frame, {view, Matrix4{}}};
}

bool close_to(float actual, float expected) {
    return std::fabs(actual - expected) < 1e-3f;
}

int step_case() {
    CameraContinuity continuity;
    if (continuity.observe(seen(10, camera_at(20.0f, 5.0f, 10.0f)))) return 10;
    auto step = continuity.observe(seen(11, camera_at(22.0f, 7.0f, 40.0f)));
    if (!step || step->frame != 11) return 11;
    if (!close_to(step->distance_m, std::sqrt(8.0f)) || !close_to(step->angle_deg, 30.0f)) {
        std::cerr << step->distance_m << ' ' << step->angle_deg << '\n';
        return 12;
    }
    if (!close_to(step->from.x, 20.0f) || !close_to(step->to.z, 7.0f) || !close_to(step->to.y, 1.5f)) return 13;
    return is_jump(*step) ? 0 : 14;
}

int gap_case() {
    CameraContinuity continuity;
    continuity.observe(seen(10, camera_at(0.0f, 0.0f, 0.0f)));
    if (continuity.observe(seen(12, camera_at(0.5f, 0.0f, 0.0f)))) return 20;
    auto step = continuity.observe(seen(13, camera_at(1.0f, 0.0f, 1.0f)));
    if (!step || !close_to(step->distance_m, 0.5f) || !close_to(step->angle_deg, 1.0f)) return 21;
    return is_jump(*step) ? 22 : 0;
}

int singular_case() {
    CameraContinuity continuity;
    continuity.observe(seen(1, camera_at(0.0f, 0.0f, 0.0f)));
    if (continuity.observe(seen(2, Matrix4{}))) return 30;
    return continuity.observe(seen(3, camera_at(0.0f, 0.0f, 0.0f))) ? 31 : 0;
}

int limits_case() {
    CameraStep step{1, 1.9f, 19.0f, {}, {}};
    if (is_jump(step)) return 40;
    step.distance_m = 2.1f;
    if (!is_jump(step)) return 41;
    step.distance_m = 0.0f;
    step.angle_deg = 21.0f;
    return is_jump(step) ? 0 : 42;
}
}

int main() {
    for (auto test_case : {step_case, gap_case, singular_case, limits_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
