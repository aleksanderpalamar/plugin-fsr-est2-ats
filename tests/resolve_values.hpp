#pragma once

#include "temporal/resolve_constants.hpp"

#include <array>
#include <cmath>

namespace resolve_test {
using Values = std::array<float, neuralfx::temporal::resolve_floats>;

inline Values game_values(float width = 1440.0f, float height = 1080.0f) {
    Values values{};
    values[16] = 1.0f;
    values[21] = 1.0f;
    values[26] = 1.0f;
    values[44] = width;
    values[45] = height;
    return values;
}

inline void turn_by(Values& values, float yaw_degrees) {
    constexpr float degrees = 3.14159265f / 180.0f;
    float c = std::cos(yaw_degrees * degrees);
    float s = std::sin(yaw_degrees * degrees);
    values[16] = c;
    values[18] = s;
    values[24] = -s;
    values[26] = c;
}

inline void move_by(Values& values, float x, float y, float z) {
    values[19] = x;
    values[23] = y;
    values[27] = z;
}

inline void jitter_by(Values& values, float x_pixels, float y_pixels) {
    values[32] = x_pixels / values[44];
    values[33] = y_pixels / values[45];
}
}
