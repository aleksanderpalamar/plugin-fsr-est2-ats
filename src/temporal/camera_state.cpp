#include "temporal/camera_state.hpp"

namespace neuralfx::temporal {
namespace {
constexpr std::size_t horizontal_offset = 2;
constexpr std::size_t vertical_offset = 6;

Matrix4 without_jitter(Matrix4 projection) {
    projection[horizontal_offset] = 0.0f;
    projection[vertical_offset] = 0.0f;
    return projection;
}
}

std::optional<CameraState> camera_state(const CameraMatrices& matrices, RenderSize size) noexcept {
    auto view_inverse = inverse(matrices.view);
    if (!view_inverse) return std::nullopt;
    Matrix4 jittered = multiply(matrices.view_projection, *view_inverse);
    auto jitter = projection_jitter(jittered, size);
    if (!jitter) return std::nullopt;
    Matrix4 projection = without_jitter(jittered);
    return CameraState{*jitter, projection, multiply(projection, matrices.view)};
}
}
