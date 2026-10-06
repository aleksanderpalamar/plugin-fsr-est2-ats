#include "raytracing/bvh.hpp"

namespace neuralfx::raytracing {
void Bvh::push_children(const Branch& branch, const PreparedRay& ray,
    double minimum, double maximum,
    std::array<Candidate, max_depth + 2>& stack, size_t& top) const noexcept {
    auto left = nodes_[branch.left].bounds.entry(ray, minimum, maximum);
    auto right = nodes_[branch.right].bounds.entry(ray, minimum, maximum);
    if (left && right) {
        bool left_first = *left <= *right;
        stack[top++] = left_first ? Candidate{branch.right, *right} : Candidate{branch.left, *left};
        stack[top++] = left_first ? Candidate{branch.left, *left} : Candidate{branch.right, *right};
        return;
    }
    if (left) stack[top++] = {branch.left, *left};
    if (right) stack[top++] = {branch.right, *right};
}

std::optional<Hit> Bvh::intersect(const Ray& ray, double minimum, double maximum) const noexcept {
    if (nodes_.empty()) return std::nullopt;
    PreparedRay prepared(ray);
    auto root = nodes_[0].bounds.entry(prepared, minimum, maximum);
    if (!root) return std::nullopt;
    std::array<Candidate, max_depth + 2> stack{};
    size_t top = 0;
    stack[top++] = {0, *root};
    std::optional<Hit> nearest;
    while (top > 0) {
        Candidate candidate = stack[--top];
        double limit = nearest ? nearest->distance : maximum;
        if (candidate.entry > limit) continue;
        const Node& node = nodes_[candidate.node];
        auto* leaf = std::get_if<Leaf>(&node.content);
        if (!leaf) {
            push_children(std::get<Branch>(node.content), prepared, minimum, limit, stack, top);
            continue;
        }
        for (size_t offset = 0; offset < leaf->count; ++offset) {
            size_t primitive = indices_[leaf->first + offset];
            auto hit = raytracing::intersect(triangles_[primitive], ray, minimum, limit, primitive);
            if (!hit) continue;
            limit = hit->distance;
            nearest = hit;
        }
    }
    return nearest;
}

bool Bvh::occluded(const Ray& ray, double minimum, double maximum) const noexcept {
    if (nodes_.empty()) return false;
    PreparedRay prepared(ray);
    auto root = nodes_[0].bounds.entry(prepared, minimum, maximum);
    if (!root) return false;
    std::array<Candidate, max_depth + 2> stack{};
    size_t top = 0;
    stack[top++] = {0, *root};
    while (top > 0) {
        Candidate candidate = stack[--top];
        const Node& node = nodes_[candidate.node];
        auto* leaf = std::get_if<Leaf>(&node.content);
        if (!leaf) {
            push_children(std::get<Branch>(node.content), prepared, minimum, maximum, stack, top);
            continue;
        }
        for (size_t offset = 0; offset < leaf->count; ++offset) {
            size_t primitive = indices_[leaf->first + offset];
            if (raytracing::intersect(triangles_[primitive], ray, minimum, maximum, primitive)) return true;
        }
    }
    return false;
}
}
