#pragma once

#include "raytracing/vec3.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <optional>

namespace neuralfx::raytracing {
inline constexpr double ray_epsilon = 1e-4;
inline constexpr double intersection_epsilon = 1e-12;

struct Aabb {
    Vec3 min{std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    Vec3 max{-std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

    void expand(Vec3 point) noexcept;
    void expand(const Aabb& other) noexcept;
};

struct Triangle {
    std::array<Vec3, 3> positions;
    std::optional<std::array<Vec3, 3>> normals;
    size_t material = 0;

    Aabb bounds() const noexcept;
    Vec3 centroid() const noexcept;
};
}
