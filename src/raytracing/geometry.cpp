#include "raytracing/geometry.hpp"

#include <algorithm>

namespace neuralfx::raytracing {
void Aabb::expand(Vec3 point) noexcept {
    for (size_t axis = 0; axis < 3; ++axis) {
        min[axis] = std::min(min[axis], point[axis]);
        max[axis] = std::max(max[axis], point[axis]);
    }
}

void Aabb::expand(const Aabb& other) noexcept {
    expand(other.min);
    expand(other.max);
}

Aabb Triangle::bounds() const noexcept {
    Aabb box;
    for (Vec3 point : positions) box.expand(point);
    return box;
}

Vec3 Triangle::centroid() const noexcept {
    return (positions[0] + positions[1] + positions[2]) / 3.0;
}
}
