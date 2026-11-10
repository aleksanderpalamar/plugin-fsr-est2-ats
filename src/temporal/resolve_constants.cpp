#include "temporal/resolve_constants.hpp"

#include <algorithm>
#include <cmath>

namespace neuralfx::temporal {
namespace {
constexpr double radians_to_degrees = 57.29577951308232;
constexpr std::size_t rows_at = 16;
constexpr std::size_t jitter_at = 32;
constexpr std::size_t size_at = 44;
constexpr std::size_t used_floats = size_at + 2;

bool all_finite(std::span<const float> values) {
    return std::all_of(values.begin(), values.end(), [](float value) { return std::isfinite(value); });
}

float row(std::span<const float> values, std::size_t index, std::size_t column) {
    return values[rows_at + index * 4 + column];
}

float translation_length(std::span<const float> values) {
    double x = row(values, 0, 3);
    double y = row(values, 1, 3);
    double z = row(values, 2, 3);
    return static_cast<float>(std::sqrt(x * x + y * y + z * z));
}

float rotation_degrees(std::span<const float> values) {
    double x = static_cast<double>(row(values, 2, 1)) - row(values, 1, 2);
    double y = static_cast<double>(row(values, 0, 2)) - row(values, 2, 0);
    double z = static_cast<double>(row(values, 1, 0)) - row(values, 0, 1);
    double sine = 0.5 * std::sqrt(x * x + y * y + z * z);
    double trace = static_cast<double>(row(values, 0, 0)) + row(values, 1, 1) + row(values, 2, 2);
    return static_cast<float>(std::atan2(sine, (trace - 1.0) * 0.5) * radians_to_degrees);
}
}

std::variant<ResolveFrame, ResolveFault> evaluate_resolve(std::span<const float> values, RenderSize expected) noexcept {
    if (values.size() < used_floats) return ResolveFault::TooShort;
    if (!all_finite(values.first(used_floats))) return ResolveFault::NotFinite;
    if (values[size_at] != static_cast<float>(expected.width) || values[size_at + 1] != static_cast<float>(expected.height))
        return ResolveFault::SizeMismatch;
    Jitter jitter{values[jitter_at] * static_cast<float>(expected.width),
        values[jitter_at + 1] * static_cast<float>(expected.height)};
    return ResolveFrame{jitter, translation_length(values), rotation_degrees(values)};
}
}
