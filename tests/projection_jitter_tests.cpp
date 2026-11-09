#include "temporal/projection_jitter.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx::temporal;

namespace {
constexpr RenderSize scene{1440, 1080};

Matrix4 projection(float x_term, float y_term) {
    return {1.2482098f, 0.0f, x_term, 0.0f,
        0.0f, 2.2190397f, y_term, 0.0f,
        0.0f, 0.0f, 0.00012499095f, 0.20002499f,
        0.0f, 0.0f, -1.0000001f, 0.0f};
}

bool near(float actual, float expected) {
    return std::fabs(actual - expected) < 1e-4f;
}

int expect_jitter(const Matrix4& matrix, float x, float y, int code) {
    auto jitter = projection_jitter(matrix, scene);
    if (!jitter) return code;
    if (near(jitter->x, x) && near(jitter->y, y)) return 0;
    std::cerr << "case " << code << " produced " << jitter->x << ',' << jitter->y << '\n';
    return code + 1;
}

int captured_frames_case() {
    if (int code = expect_jitter(projection(0.0006537968f, -0.0007760036f), -0.470703f, -0.419042f, 10)) return code;
    return expect_jitter(projection(-0.0003946444f, 2.058877e-05f), 0.284144f, 0.011118f, 20);
}

int left_handed_case() {
    auto matrix = projection(0.0006537968f, -0.0007760036f);
    matrix[14] = 1.0f;
    return expect_jitter(matrix, 0.470703f, 0.419042f, 30);
}

int rejection_case() {
    Matrix4 orthographic{0.0005917546f, 0.0f, 0.0f, -0.7655108f,
        0.0f, 0.00078285206f, 0.0f, -0.72001624f,
        0.0f, 0.0f, -0.0002791546f, 1.0746974f,
        0.0f, 0.0f, 0.0f, 1.0f};
    if (projection_jitter(orthographic, scene)) return 40;
    if (projection_jitter(projection(0.0f, 0.0f), RenderSize{0, 1080})) return 41;
    if (projection_jitter(projection(0.002f, 0.0f), scene)) return 42;
    auto still = projection_jitter(projection(0.0f, 0.0f), scene);
    return still && still->x == 0.0f && still->y == 0.0f ? 0 : 43;
}

int expect_fault(const Matrix4& matrix, RenderSize size, CameraFault fault, int code) {
    auto result = evaluate_jitter(matrix, size);
    const auto* found = std::get_if<CameraFault>(&result);
    return found && *found == fault ? 0 : code;
}

int fault_case() {
    Matrix4 orthographic = projection(0.0f, 0.0f);
    orthographic[14] = 0.0f;
    orthographic[15] = 1.0f;
    if (int code = expect_fault(orthographic, scene, CameraFault::NotPerspective, 50)) return code;
    if (int code = expect_fault(projection(0.0f, 0.0f), RenderSize{1440, 0}, CameraFault::EmptySize, 51)) return code;
    if (int code = expect_fault(projection(0.0f, 0.002f), scene, CameraFault::JitterOutOfRange, 52)) return code;
    auto valid = evaluate_jitter(projection(0.0006537968f, -0.0007760036f), scene);
    return std::holds_alternative<Jitter>(valid) ? 0 : 53;
}
}

int main() {
    for (auto test_case : {captured_frames_case, left_handed_case, rejection_case, fault_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
