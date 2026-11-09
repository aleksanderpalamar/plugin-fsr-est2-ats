#pragma once

#include <array>
#include <optional>

namespace neuralfx::temporal {
using Matrix4 = std::array<float, 16>;

Matrix4 multiply(const Matrix4& left, const Matrix4& right) noexcept;
std::optional<Matrix4> inverse(const Matrix4& matrix) noexcept;
}
