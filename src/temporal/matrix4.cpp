#include "temporal/matrix4.hpp"

#include <cmath>
#include <utility>

namespace neuralfx::temporal {
namespace {
using Augmented = std::array<std::array<double, 8>, 4>;

constexpr double singular_limit = 1e-12;

Augmented augment(const Matrix4& matrix) {
    Augmented rows{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) rows[row][column] = matrix[row * 4 + column];
        rows[row][4 + row] = 1.0;
    }
    return rows;
}

std::size_t pivot_row(const Augmented& rows, std::size_t column) {
    std::size_t best = column;
    for (std::size_t row = column + 1; row < 4; ++row)
        if (std::fabs(rows[row][column]) > std::fabs(rows[best][column])) best = row;
    return best;
}

void eliminate(Augmented& rows, std::size_t column) {
    double scale = rows[column][column];
    for (auto& value : rows[column]) value /= scale;
    for (std::size_t row = 0; row < 4; ++row) {
        if (row == column) continue;
        double factor = rows[row][column];
        for (std::size_t index = 0; index < 8; ++index) rows[row][index] -= factor * rows[column][index];
    }
}

Matrix4 right_half(const Augmented& rows) {
    Matrix4 result{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t column = 0; column < 4; ++column)
            result[row * 4 + column] = static_cast<float>(rows[row][4 + column]);
    return result;
}
}

Matrix4 multiply(const Matrix4& left, const Matrix4& right) noexcept {
    Matrix4 result{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            double sum = 0.0;
            for (std::size_t index = 0; index < 4; ++index)
                sum += static_cast<double>(left[row * 4 + index]) * right[index * 4 + column];
            result[row * 4 + column] = static_cast<float>(sum);
        }
    }
    return result;
}

std::optional<Matrix4> inverse(const Matrix4& matrix) noexcept {
    Augmented rows = augment(matrix);
    for (std::size_t column = 0; column < 4; ++column) {
        std::swap(rows[column], rows[pivot_row(rows, column)]);
        if (std::fabs(rows[column][column]) < singular_limit) return std::nullopt;
        eliminate(rows, column);
    }
    return right_half(rows);
}
}
