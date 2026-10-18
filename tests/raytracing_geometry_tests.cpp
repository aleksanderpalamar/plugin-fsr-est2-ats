#include "raytracing/bvh.hpp"
#include "raytracing/geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <variant>
#include <vector>

using namespace neuralfx::raytracing;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(Vec3 actual, Vec3 expected) {
    return length(actual - expected) < 1e-12;
}

Triangle triangle(double x, double z) {
    return {std::array<Vec3, 3>{{{x - 1.0, -1.0, z}, {x + 1.0, -1.0, z}, {x, 1.0, z}}},
        std::nullopt, 0};
}

bool contains(const Aabb& outer, const Aabb& inner) {
    for (size_t axis = 0; axis < 3; ++axis)
        if (inner.min[axis] < outer.min[axis] || inner.max[axis] > outer.max[axis]) return false;
    return true;
}

void triangle_cases() {
    Triangle surface = triangle(2.0, -3.0);
    Aabb bounds = surface.bounds();
    require(near(bounds.min, {1, -1, -3}) && near(bounds.max, {3, 1, -3}), "triangle bounds");
    require(near(surface.centroid(), {2, -1.0 / 3.0, -3}), "triangle centroid");
    Aabb merged = bounds;
    merged.expand(triangle(-5.0, -1.0).bounds());
    require(near(merged.min, {-6, -1, -3}) && near(merged.max, {3, 1, -1}), "bounds merge");
}

void structure_cases(const Bvh& bvh, size_t triangle_count) {
    auto order = bvh.primitive_order();
    std::vector<size_t> sorted(order.begin(), order.end());
    std::sort(sorted.begin(), sorted.end());
    for (size_t index = 0; index < sorted.size(); ++index)
        require(sorted[index] == index, "primitive order is a permutation");
    std::vector<unsigned> covered(triangle_count, 0);
    auto nodes = bvh.nodes();
    for (const Bvh::Node& node : nodes) {
        if (const auto* branch = std::get_if<Bvh::Branch>(&node.content)) {
            require(contains(node.bounds, nodes[branch->left].bounds), "left child inside parent");
            require(contains(node.bounds, nodes[branch->right].bounds), "right child inside parent");
            continue;
        }
        const auto& leaf = std::get<Bvh::Leaf>(node.content);
        require(leaf.count > 0 && leaf.first + leaf.count <= triangle_count, "leaf range valid");
        for (size_t offset = 0; offset < leaf.count; ++offset) {
            size_t primitive = order[leaf.first + offset];
            ++covered[leaf.first + offset];
            require(contains(node.bounds, bvh.triangles()[primitive].bounds()), "leaf bounds cover triangles");
        }
    }
    require(std::all_of(covered.begin(), covered.end(), [](unsigned count) { return count == 1; }),
        "leaves partition the primitives");
}

void bvh_cases() {
    std::vector<Triangle> triangles;
    for (int layer = 0; layer < 3; ++layer)
        for (int index = -6; index <= 6; ++index)
            triangles.push_back(triangle(index * 2.5, -3.0 - layer * 1.5));
    Bvh bvh(triangles);
    require(bvh.node_count() > 1, "BVH has branches");
    require(bvh.triangles().size() == triangles.size(), "BVH keeps every triangle");
    structure_cases(bvh, triangles.size());
    bvh.rebuild({triangle(10, -3)});
    require(bvh.node_count() == 1 && bvh.triangles().size() == 1, "rebuild replaces geometry");
    require(near(bvh.triangles()[0].positions[0], {9, -1, -3}), "rebuild keeps the new triangle");
    Bvh empty({});
    require(empty.node_count() == 0 && empty.primitive_order().empty(), "empty BVH");
}
}

int main() {
    try {
        triangle_cases();
        bvh_cases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
