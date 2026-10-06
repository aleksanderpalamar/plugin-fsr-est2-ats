#pragma once

#include "raytracing/geometry.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

namespace neuralfx::raytracing {
class Bvh {
public:
    explicit Bvh(std::vector<Triangle> triangles);
    void rebuild(std::vector<Triangle> triangles);
    std::optional<Hit> intersect(const Ray& ray, double minimum, double maximum) const noexcept;
    bool occluded(const Ray& ray, double minimum, double maximum) const noexcept;
    size_t node_count() const noexcept { return nodes_.size(); }

private:
    static constexpr size_t max_depth = 48;
    static constexpr size_t leaf_size = 4;

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
    struct Candidate {
        size_t node;
        double entry;
    };

    size_t build(size_t first, size_t count, size_t depth);
    void push_children(const Branch& branch, const PreparedRay& ray,
        double minimum, double maximum,
        std::array<Candidate, max_depth + 2>& stack, size_t& top) const noexcept;

    std::vector<Triangle> triangles_;
    std::vector<size_t> indices_;
    std::vector<Node> nodes_;
};
}
