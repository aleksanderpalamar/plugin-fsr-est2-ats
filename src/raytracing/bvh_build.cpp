#include "raytracing/bvh.hpp"

#include <algorithm>
#include <numeric>
#include <utility>

namespace neuralfx::raytracing {
Bvh::Bvh(std::vector<Triangle> triangles) { rebuild(std::move(triangles)); }

void Bvh::rebuild(std::vector<Triangle> triangles) {
    triangles_ = std::move(triangles);
    indices_.resize(triangles_.size());
    std::iota(indices_.begin(), indices_.end(), 0);
    nodes_.clear();
    if (triangles_.empty()) return;
    nodes_.reserve(triangles_.size());
    build(0, triangles_.size(), 0);
}

size_t Bvh::build(size_t first, size_t count, size_t depth) {
    Aabb bounds;
    Aabb centroids;
    for (size_t offset = 0; offset < count; ++offset) {
        const Triangle& triangle = triangles_[indices_[first + offset]];
        bounds.expand(triangle.bounds());
        centroids.expand(triangle.centroid());
    }
    size_t node = nodes_.size();
    nodes_.push_back({bounds, Leaf{first, count}});
    if (count <= leaf_size || depth >= max_depth) return node;
    Vec3 extent = centroids.max - centroids.min;
    size_t axis = extent.y > extent.x ? 1 : 0;
    if (extent.z > extent[axis]) axis = 2;
    if (extent[axis] <= intersection_epsilon) return node;
    size_t middle = first + count / 2;
    std::nth_element(indices_.begin() + first, indices_.begin() + middle,
        indices_.begin() + first + count, [this, axis](size_t left, size_t right) {
            double left_value = triangles_[left].centroid()[axis];
            double right_value = triangles_[right].centroid()[axis];
            return left_value == right_value ? left < right : left_value < right_value;
        });
    size_t left = build(first, middle - first, depth + 1);
    size_t right = build(middle, count - (middle - first), depth + 1);
    nodes_[node].content = Branch{left, right};
    return node;
}
}
