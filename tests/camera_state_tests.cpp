#include "temporal/camera_state.hpp"
#include "temporal/matrix4.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx::temporal;

namespace {
constexpr RenderSize scene{1440, 1080};

constexpr Matrix4 view{-0.47095215f, 0.0f, -0.8821588f, -46.862404f,
    0.0f, 1.0f, 0.0f, 10.617724f,
    0.8821588f, 0.0f, -0.47095215f, -48.97045f,
    0.0f, 0.0f, 0.0f, 1.0f};

constexpr Matrix4 first_frame{-0.5872703f, 0.0f, -1.1014271f, -58.526123f,
    -0.0006845583f, 2.2190397f, 0.0003654605f, 23.599154f,
    0.00011026186f, 0.0f, -5.8864753e-05f, 0.19390413f,
    -0.8821588f, 0.0f, 0.47095215f, 48.97045f};

constexpr Matrix4 second_frame{-0.5881952f, 0.0f, -1.1009333f, -58.47478f,
    1.8162564e-05f, 2.2190397f, -9.696325e-06f, 23.560148f,
    0.00011026186f, 0.0f, -5.8864753e-05f, 0.19390413f,
    -0.8821588f, 0.0f, 0.47095215f, 48.97045f};

bool near(float actual, float expected, float tolerance) {
    return std::fabs(actual - expected) < tolerance;
}

int jitter_case() {
    auto first = camera_state({view, first_frame}, scene);
    auto second = camera_state({view, second_frame}, scene);
    if (!first || !second) return 10;
    if (!near(first->jitter.x, -0.4707f, 2e-3f) || !near(first->jitter.y, -0.4190f, 2e-3f)) return 11;
    if (!near(second->jitter.x, 0.2841f, 2e-3f) || !near(second->jitter.y, 0.0111f, 2e-3f)) return 12;
    return 0;
}

int unjittered_case() {
    auto first = camera_state({view, first_frame}, scene);
    auto second = camera_state({view, second_frame}, scene);
    if (!first || !second) return 20;
    for (std::size_t index = 0; index < 16; ++index) {
        float scale = std::fmax(1.0f, std::fabs(first->view_projection[index]));
        if (!near(first->view_projection[index], second->view_projection[index], 1e-4f * scale)) {
            std::cerr << "element " << index << ": " << first->view_projection[index] << " vs "
                      << second->view_projection[index] << '\n';
            return 21;
        }
    }
    return near(first->view_projection[4], 0.0f, 1e-5f) && near(first->projection[2], 0.0f, 1e-7f) ? 0 : 22;
}

int rejection_case() {
    Matrix4 singular{};
    if (camera_state({singular, first_frame}, scene)) return 30;
    Matrix4 orthographic = first_frame;
    orthographic[12] = 0.0f;
    orthographic[14] = 0.0f;
    orthographic[15] = 1.0f;
    return camera_state({view, orthographic}, scene) ? 31 : 0;
}

int fault_case() {
    auto singular = evaluate_camera({Matrix4{}, first_frame}, scene);
    const auto* rejected = std::get_if<CameraRejection>(&singular);
    if (!rejected || rejected->fault != CameraFault::SingularView) return 50;
    Matrix4 orthographic = first_frame;
    orthographic[12] = 0.0f;
    orthographic[14] = 0.0f;
    orthographic[15] = 1.0f;
    auto flat = evaluate_camera({view, orthographic}, scene);
    rejected = std::get_if<CameraRejection>(&flat);
    if (!rejected || rejected->fault != CameraFault::NotPerspective) return 51;
    if (!near(rejected->projection[15], 1.0f, 1e-4f) || !near(rejected->projection[0], 1.2482098f, 1e-4f)) return 52;
    auto valid = evaluate_camera({view, first_frame}, scene);
    return std::holds_alternative<CameraState>(valid) ? 0 : 53;
}

int inverse_case() {
    auto inverted = inverse(view);
    if (!inverted) return 40;
    auto identity = multiply(view, *inverted);
    for (std::size_t index = 0; index < 16; ++index) {
        float expected = index % 5 == 0 ? 1.0f : 0.0f;
        if (!near(identity[index], expected, 1e-5f)) return 41;
    }
    return inverse(Matrix4{}) ? 42 : 0;
}
}

int main() {
    for (auto test_case : {inverse_case, jitter_case, unjittered_case, rejection_case, fault_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
