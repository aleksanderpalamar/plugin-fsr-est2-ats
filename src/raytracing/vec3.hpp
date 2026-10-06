#pragma once

#include <cmath>
#include <cstddef>

namespace neuralfx::raytracing {
struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    double& operator[](size_t axis) noexcept { return axis == 0 ? x : axis == 1 ? y : z; }
    double operator[](size_t axis) const noexcept { return axis == 0 ? x : axis == 1 ? y : z; }
};

inline Vec3 operator+(Vec3 left, Vec3 right) noexcept {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

inline Vec3 operator-(Vec3 left, Vec3 right) noexcept {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

inline Vec3 operator-(Vec3 value) noexcept {
    return {-value.x, -value.y, -value.z};
}

inline Vec3 operator*(Vec3 value, double scale) noexcept {
    return {value.x * scale, value.y * scale, value.z * scale};
}

inline Vec3 operator*(double scale, Vec3 value) noexcept { return value * scale; }

inline Vec3 operator/(Vec3 value, double scale) noexcept { return value * (1.0 / scale); }

inline Vec3 multiply(Vec3 left, Vec3 right) noexcept {
    return {left.x * right.x, left.y * right.y, left.z * right.z};
}

inline double dot(Vec3 left, Vec3 right) noexcept {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

inline Vec3 cross(Vec3 left, Vec3 right) noexcept {
    return {left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x};
}

inline double length(Vec3 value) noexcept { return std::sqrt(dot(value, value)); }

inline Vec3 normalized(Vec3 value) noexcept {
    double magnitude = length(value);
    return magnitude > 0.0 ? value / magnitude : Vec3{};
}
}
