#include "temporal/motion_reprojection.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx::temporal;

namespace {
constexpr RenderSize scene{1440, 1080};

constexpr Matrix4 projection{1.2482098f, 0.0f, 0.0f, 0.0f,
    0.0f, 2.2190397f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.00012499095f, 0.20002499f,
    0.0f, 0.0f, -1.0f, 0.0f};

struct Point {
    float x;
    float y;
    float depth;
};

Matrix4 translation(float x, float y, float z) {
    return {1.0f, 0.0f, 0.0f, -x, 0.0f, 1.0f, 0.0f, -y, 0.0f, 0.0f, 1.0f, -z, 0.0f, 0.0f, 0.0f, 1.0f};
}

CameraState camera(const Matrix4& view, Jitter jitter) {
    return {jitter, projection, multiply(projection, view)};
}

Point project(const CameraState& state, float x, float y, float z) {
    const auto& m = state.view_projection;
    float clip[4];
    for (int row = 0; row < 4; ++row)
        clip[row] = m[row * 4] * x + m[row * 4 + 1] * y + m[row * 4 + 2] * z + m[row * 4 + 3];
    float ndc_x = clip[0] / clip[3];
    float ndc_y = clip[1] / clip[3];
    return {(ndc_x + 1.0f) * 0.5f * scene.width, (1.0f - ndc_y) * 0.5f * scene.height, clip[2] / clip[3]};
}

bool near(float actual, float expected) {
    return std::fabs(actual - expected) < 2e-2f;
}

int expect_motion(const CameraState& previous, const CameraState& current, float x, float y, float z, int code) {
    auto constants = motion_constants(previous, current, scene);
    if (!constants) return code;
    Point now = project(current, x, y, z);
    Point before = project(previous, x, y, z);
    auto motion = camera_motion(*constants, now.x + current.jitter.x, now.y + current.jitter.y, now.depth);
    if (!motion) return code + 1;
    if (near(motion->x, before.x - now.x) && near(motion->y, before.y - now.y)) return 0;
    std::cerr << "case " << code << " produced " << motion->x << ',' << motion->y << " expected "
              << before.x - now.x << ',' << before.y - now.y << '\n';
    return code + 2;
}

int static_camera_case() {
    auto previous = camera(translation(0.0f, 0.0f, 0.0f), {0.3f, -0.2f});
    auto current = camera(translation(0.0f, 0.0f, 0.0f), {-0.47f, -0.42f});
    if (int code = expect_motion(previous, current, 2.0f, 1.0f, -10.0f, 10)) return code;
    return expect_motion(previous, current, -40.0f, 3.0f, -900.0f, 20);
}

int moving_camera_case() {
    auto previous = camera(translation(0.0f, 0.0f, 0.0f), {0.28f, 0.01f});
    auto current = camera(translation(0.5f, 0.0f, -1.5f), {-0.47f, -0.42f});
    if (int code = expect_motion(previous, current, 2.0f, 1.0f, -10.0f, 30)) return code;
    return expect_motion(previous, current, -3.0f, -1.0f, -25.0f, 40);
}

int rejection_case() {
    auto current = camera(translation(0.0f, 0.0f, 0.0f), {});
    current.view_projection = {};
    if (motion_constants(camera(translation(0.0f, 0.0f, 0.0f), {}), current, scene)) return 50;
    auto previous = camera(translation(0.0f, 0.0f, -20.0f), {});
    auto constants = motion_constants(previous, camera(translation(0.0f, 0.0f, 0.0f), {}), scene);
    if (!constants) return 51;
    Point behind = project(camera(translation(0.0f, 0.0f, 0.0f), {}), 0.0f, 0.0f, -10.0f);
    return camera_motion(*constants, behind.x, behind.y, behind.depth) ? 52 : 0;
}
}

int main() {
    for (auto test_case : {static_camera_case, moving_camera_case, rejection_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
