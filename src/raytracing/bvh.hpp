#pragma once

#include "raytracing/geometry.hpp"

#include <cstddef>
#include <span>
#include <variant>
#include <vector>

namespace neuralfx::raytracing {
class Bvh {
public:
    struct Branch {
        size_t left;
        size_t right;
    };
    struct Leaf {
        size_t first;
        size_t count;
    };
    struct Node {
        Aabb bounds;
        std::variant<Branch, Leaf> content;
    };

    explicit Bvh(std::vector<Triangle> triangles);
    void rebuild(std::vector<Triangle> triangles);
    size_t node_count() const noexcept { return nodes_.size(); }
    std::span<const Node> nodes() const noexcept { return nodes_; }
    std::span<const size_t> primitive_order() const noexcept { return indices_; }
    std::span<const Triangle> triangles() const noexcept { return triangles_; }

private:
    static constexpr size_t max_depth = 48;
    static constexpr size_t leaf_size = 4;

    size_t build(size_t first, size_t count, size_t depth);

    std::vector<Triangle> triangles_;
    std::vector<size_t> indices_;
    std::vector<Node> nodes_;
};
}
