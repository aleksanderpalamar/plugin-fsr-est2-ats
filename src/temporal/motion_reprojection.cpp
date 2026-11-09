#include "temporal/motion_reprojection.hpp"

#include <array>

namespace neuralfx::temporal {
namespace {
constexpr float minimum_w = 1e-6f;

std::array<float, 4> transform(const Matrix4& matrix, const std::array<float, 4>& vector) {
    std::array<float, 4> result{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t column = 0; column < 4; ++column)
            result[row] += matrix[row * 4 + column] * vector[column];
    return result;
}
}

std::optional<MotionConstants> motion_constants(const CameraState& previous, const CameraState& current,
    RenderSize size) noexcept {
    if (size.width == 0 || size.height == 0) return std::nullopt;
    auto current_inverse = inverse(current.view_projection);
    if (!current_inverse) return std::nullopt;
    return MotionConstants{multiply(previous.view_projection, *current_inverse), current.jitter, size};
}

std::optional<PixelMotion> camera_motion(const MotionConstants& constants, float x, float y, float depth) noexcept {
    float width = static_cast<float>(constants.size.width);
    float height = static_cast<float>(constants.size.height);
    float current_x = x - constants.jitter.x;
    float current_y = y - constants.jitter.y;
    auto clip = transform(constants.reprojection,
        {current_x / width * 2.0f - 1.0f, 1.0f - current_y / height * 2.0f, depth, 1.0f});
    if (clip[3] < minimum_w) return std::nullopt;
    float previous_x = (clip[0] / clip[3] + 1.0f) * 0.5f * width;
    float previous_y = (1.0f - clip[1] / clip[3]) * 0.5f * height;
    return PixelMotion{previous_x - current_x, previous_y - current_y};
}
}
