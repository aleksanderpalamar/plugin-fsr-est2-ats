#include "temporal/projection_jitter.hpp"

#include <cmath>

namespace neuralfx::temporal {
namespace {
constexpr float tolerance = 1e-3f;
constexpr float max_offset = 0.5f + tolerance;

float at(const Matrix4& matrix, int row, int column) {
    return matrix[static_cast<std::size_t>(row * 4 + column)];
}

bool is_zero(float value) {
    return std::fabs(value) < tolerance;
}

std::optional<float> depth_sign(const Matrix4& matrix) {
    float w = at(matrix, 3, 2);
    if (!is_zero(at(matrix, 3, 0)) || !is_zero(at(matrix, 3, 1)) || !is_zero(at(matrix, 3, 3)))
        return std::nullopt;
    if (!is_zero(std::fabs(w) - 1.0f)) return std::nullopt;
    return w;
}

bool within_pixel(Jitter jitter) {
    return std::fabs(jitter.x) <= max_offset && std::fabs(jitter.y) <= max_offset;
}
}

std::variant<Jitter, CameraFault> evaluate_jitter(const Matrix4& projection, RenderSize size) noexcept {
    if (size.width == 0 || size.height == 0) return CameraFault::EmptySize;
    auto w = depth_sign(projection);
    if (!w) return CameraFault::NotPerspective;
    Jitter jitter{at(projection, 0, 2) / *w * static_cast<float>(size.width) * 0.5f,
        -at(projection, 1, 2) / *w * static_cast<float>(size.height) * 0.5f};
    if (!within_pixel(jitter)) return CameraFault::JitterOutOfRange;
    return jitter;
}

std::optional<Jitter> projection_jitter(const Matrix4& projection, RenderSize size) noexcept {
    auto result = evaluate_jitter(projection, size);
    if (const auto* jitter = std::get_if<Jitter>(&result)) return *jitter;
    return std::nullopt;
}
}
