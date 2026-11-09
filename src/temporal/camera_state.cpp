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

std::variant<CameraState, CameraRejection> evaluate_camera(const CameraMatrices& matrices, RenderSize size) noexcept {
    auto view_inverse = inverse(matrices.view);
    if (!view_inverse) return CameraRejection{CameraFault::SingularView, {}};
    Matrix4 jittered = multiply(matrices.view_projection, *view_inverse);
    auto jitter = evaluate_jitter(jittered, size);
    if (const auto* fault = std::get_if<CameraFault>(&jitter)) return CameraRejection{*fault, jittered};
    Matrix4 projection = without_jitter(jittered);
    return CameraState{std::get<Jitter>(jitter), projection, multiply(projection, matrices.view)};
}

std::optional<CameraState> camera_state(const CameraMatrices& matrices, RenderSize size) noexcept {
    auto result = evaluate_camera(matrices, size);
    if (auto* state = std::get_if<CameraState>(&result)) return *state;
    return std::nullopt;
}
}
