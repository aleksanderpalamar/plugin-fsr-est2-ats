#include "raytracing/geometry.hpp"

#include <algorithm>
#include <cmath>

namespace neuralfx::raytracing {
PreparedRay::PreparedRay(const Ray& source) noexcept : ray(source) {
    for (size_t axis = 0; axis < 3; ++axis) {
        parallel[axis] = ray.direction[axis] == 0.0;
        reciprocal[axis] = parallel[axis] ? 0.0 : 1.0 / ray.direction[axis];
    }
}

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

std::optional<double> Aabb::entry(const Ray& ray, double minimum, double maximum) const noexcept {
    return entry(PreparedRay(ray), minimum, maximum);
}

std::optional<double> Aabb::entry(const PreparedRay& prepared, double minimum, double maximum) const noexcept {
    if (maximum < minimum) return std::nullopt;
    for (size_t axis = 0; axis < 3; ++axis) {
        if (min[axis] > max[axis]) return std::nullopt;
        double origin = prepared.ray.origin[axis];
        if (prepared.parallel[axis] && (origin < min[axis] || origin > max[axis]))
            return std::nullopt;
        if (prepared.parallel[axis]) continue;
        double first = (min[axis] - origin) * prepared.reciprocal[axis];
        double last = (max[axis] - origin) * prepared.reciprocal[axis];
        if (first > last) std::swap(first, last);
        minimum = std::max(minimum, first);
        maximum = std::min(maximum, last);
        if (maximum < minimum) return std::nullopt;
    }
    return minimum;
}

Aabb Triangle::bounds() const noexcept {
    Aabb box;
    for (Vec3 point : positions) box.expand(point);
    return box;
}

Vec3 Triangle::centroid() const noexcept {
    return (positions[0] + positions[1] + positions[2]) / 3.0;
}

std::optional<Hit> intersect(const Triangle& triangle, const Ray& ray,
    double minimum, double maximum, size_t primitive) noexcept {
    Vec3 first_edge = triangle.positions[1] - triangle.positions[0];
    Vec3 second_edge = triangle.positions[2] - triangle.positions[0];
    Vec3 perpendicular = cross(ray.direction, second_edge);
    double determinant = dot(first_edge, perpendicular);
    if (std::abs(determinant) < intersection_epsilon) return std::nullopt;
    double inverse = 1.0 / determinant;
    Vec3 offset = ray.origin - triangle.positions[0];
    double u = dot(offset, perpendicular) * inverse;
    if (u < 0.0 || u > 1.0) return std::nullopt;
    Vec3 cross_offset = cross(offset, first_edge);
    double v = dot(ray.direction, cross_offset) * inverse;
    if (v < 0.0 || u + v > 1.0) return std::nullopt;
    double distance = dot(second_edge, cross_offset) * inverse;
    if (distance < minimum || distance > maximum) return std::nullopt;
    Vec3 face = normalized(cross(first_edge, second_edge));
    if (length(face) == 0.0) return std::nullopt;
    Vec3 barycentric{1.0 - u - v, u, v};
    Vec3 normal = face;
    if (triangle.normals) {
        normal = normalized((*triangle.normals)[0] * barycentric.x +
            (*triangle.normals)[1] * u + (*triangle.normals)[2] * v);
        if (length(normal) == 0.0) normal = face;
    }
    if (dot(normal, ray.direction) > 0.0) normal = -normal;
    return Hit{distance, ray.at(distance), normal, barycentric, primitive, triangle.material};
}
}
