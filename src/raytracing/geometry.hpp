#pragma once

#include "raytracing/vec3.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <optional>

namespace neuralfx::raytracing {
inline constexpr double ray_epsilon = 1e-4;
inline constexpr double intersection_epsilon = 1e-12;

struct Ray {
    Vec3 origin;
    Vec3 direction;

    Vec3 at(double distance) const noexcept { return origin + direction * distance; }
};

struct PreparedRay {
    Ray ray;
    Vec3 reciprocal;
    std::array<bool, 3> parallel{};

    explicit PreparedRay(const Ray& source) noexcept;
};

struct Hit {
    double distance = 0.0;
    Vec3 position;
    Vec3 normal;
    Vec3 barycentric;
    size_t primitive = 0;
    size_t material = 0;
};

struct Aabb {
    Vec3 min{std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    Vec3 max{-std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

    void expand(Vec3 point) noexcept;
    void expand(const Aabb& other) noexcept;
    std::optional<double> entry(const Ray& ray, double min_distance, double max_distance) const noexcept;
    std::optional<double> entry(const PreparedRay& ray, double min_distance, double max_distance) const noexcept;
};

struct Triangle {
    std::array<Vec3, 3> positions;
    std::optional<std::array<Vec3, 3>> normals;
    size_t material = 0;

    Aabb bounds() const noexcept;
    Vec3 centroid() const noexcept;
};

std::optional<Hit> intersect(const Triangle& triangle, const Ray& ray,
    double min_distance, double max_distance, size_t primitive) noexcept;
}
