#include "raytracing/bvh.hpp"
#include "raytracing/geometry.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace neuralfx::raytracing;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(double actual, double expected) {
    return std::abs(actual - expected) < 1e-8;
}

Triangle triangle(double x, double z, size_t material = 0) {
    return {std::array<Vec3, 3>{{{x - 1.0, -1.0, z}, {x + 1.0, -1.0, z}, {x, 1.0, z}}},
        std::nullopt, material};
}

void triangle_cases() {
    auto surface = triangle(0.0, -3.0, 2);
    auto center = intersect(surface, {{0, 0, 0}, {0, 0, -1}}, ray_epsilon, 100.0, 7);
    require(center.has_value(), "triangle center hit");
    require(near(center->distance, 3.0), "triangle distance");
    require(near(center->barycentric.x + center->barycentric.y + center->barycentric.z, 1.0), "barycentric sum");
    require(center->primitive == 7 && center->material == 2, "triangle identifiers");
    require(dot(center->normal, {0, 0, -1}) < 0.0, "normal faces incoming ray");
    require(intersect(surface, {{-1, -1, 0}, {0, 0, -1}}, ray_epsilon, 100.0, 0).has_value(), "triangle edge hit");
    require(!intersect(surface, {{3, 0, 0}, {0, 0, -1}}, ray_epsilon, 100.0, 0), "triangle miss");
    require(!intersect(surface, {{0, 0, -4}, {0, 0, -1}}, ray_epsilon, 100.0, 0), "triangle behind ray");
    require(!intersect(surface, {{0, 0, 0}, {1, 0, 0}}, ray_epsilon, 100.0, 0), "parallel ray");
    require(!intersect(surface, {{0, 0, 0}, {0, 0, -1}}, 3.1, 100.0, 0), "minimum distance");
    surface.normals = std::array<Vec3, 3>{{{0, 0, 1}, {0, 1, 1}, {0, 0, 1}}};
    auto smooth = intersect(surface, {{0, 0, 0}, {0, 0, -1}}, ray_epsilon, 100.0, 0);
    require(smooth && smooth->normal.y > 0.0, "interpolated normal");
}

void box_cases() {
    Aabb box{{-1, -1, -1}, {1, 1, 1}};
    require(box.entry({{0, 0, 3}, {0, 0, -1}}, 0.0, 100.0).has_value(), "box hit");
    require(!box.entry({{2, 0, 3}, {0, 0, -1}}, 0.0, 100.0), "parallel outside box");
    require(box.entry({{0, 0, 0}, {1, 0, 0}}, 0.0, 100.0).has_value(), "origin inside box");
    require(box.entry({{3, 0, 0}, {-1, 0, 0}}, 0.0, 100.0).has_value(), "negative direction");
    require(!box.entry({{3, 3, 0}, {-1, 0, 0}}, 0.0, 100.0), "box miss");
    require(!box.entry({{0, 0, 3}, {0, 0, -1}}, 0.0, 1.0), "bounded box interval");
    require(!box.entry({{0, 0, 0}, {0, 0, 0}}, 2.0, 1.0), "invalid interval");
    require(box.entry({{-2, 0, 0}, {1e-13, 0, 0}}, 0.0, 4e13).has_value(),
        "small nonzero direction");
}

std::optional<Hit> brute_force(const std::vector<Triangle>& triangles,
    const Ray& ray, double maximum = 1000.0) {
    std::optional<Hit> closest;
    for (size_t index = 0; index < triangles.size(); ++index) {
        auto hit = intersect(triangles[index], ray, ray_epsilon,
            closest ? closest->distance : maximum, index);
        if (hit) closest = hit;
    }
    return closest;
}

void bvh_cases() {
    std::vector<Triangle> triangles;
    for (int index = -4; index <= 4; ++index)
        triangles.push_back(triangle(index * 3.0, -3.0 - (index & 1)));
    Bvh bvh(triangles);
    require(bvh.node_count() > 1, "BVH has branches");
    for (int index = -20; index <= 20; ++index) {
        Ray ray{{0, 0, 0}, normalized({index * 0.04, 0, -1})};
        auto accelerated = bvh.intersect(ray, ray_epsilon, 1000.0);
        auto reference = brute_force(triangles, ray);
        require(accelerated.has_value() == reference.has_value(), "BVH hit agreement");
        if (!reference) continue;
        require(accelerated->primitive == reference->primitive, "BVH primitive agreement");
        require(near(accelerated->distance, reference->distance), "BVH distance agreement");
    }
    Ray center{{0, 0, 0}, {0, 0, -1}};
    require(bvh.occluded(center, ray_epsilon, 4.0), "occlusion blocker");
    require(!bvh.occluded(center, ray_epsilon, 2.0), "occlusion beyond limit");
    require(!bvh.occluded({{50, 0, 0}, {0, 0, -1}}, ray_epsilon, 100.0), "no blocker");
    bvh.rebuild({triangle(10, -3)});
    require(!bvh.intersect(center, ray_epsilon, 100.0), "old geometry removed after rebuild");
    require(bvh.intersect({{10, 0, 0}, {0, 0, -1}}, ray_epsilon, 100.0).has_value(),
        "new geometry visible after rebuild");
    Bvh empty({});
    require(!empty.intersect(center, ray_epsilon, 100.0), "empty BVH");
    require(empty.node_count() == 0, "empty BVH nodes");
}

void compare_bvh_hit(const Bvh& bvh, const std::vector<Triangle>& triangles,
    const Ray& ray, size_t& hits, size_t& misses) {
    auto accelerated = bvh.intersect(ray, ray_epsilon, 100.0);
    auto reference = brute_force(triangles, ray, 100.0);
    require(accelerated.has_value() == reference.has_value(), "3D BVH hit agreement");
    if (!reference) {
        ++misses;
        require(!bvh.occluded(ray, ray_epsilon, 100.0), "3D BVH miss occlusion");
        return;
    }
    ++hits;
    require(accelerated->primitive == reference->primitive, "3D BVH primitive agreement");
    require(near(accelerated->distance, reference->distance), "3D BVH distance agreement");
    require(!bvh.occluded(ray, ray_epsilon, reference->distance - 0.01),
        "3D BVH occlusion before nearest hit");
    require(bvh.occluded(ray, ray_epsilon, reference->distance + 0.01),
        "3D BVH occlusion after nearest hit");
}

void bvh_3d_cases() {
    std::vector<Triangle> triangles;
    for (int layer = 0; layer < 3; ++layer) {
        for (int y = -3; y <= 3; ++y) {
            for (int x = -4; x <= 4; ++x) {
                double cx = x * 1.4;
                double cy = y * 1.4;
                double cz = -4.0 - layer * 1.6;
                triangles.push_back({{{{cx - 0.45, cy - 0.40, cz - 0.10},
                    {cx + 0.45, cy - 0.35, cz + 0.15},
                    {cx + 0.05, cy + 0.50, cz - 0.05}}}, std::nullopt, 0});
            }
        }
    }
    Bvh bvh(triangles);
    require(triangles.size() == 189 && bvh.node_count() > 1, "3D BVH branches");
    const std::array<Vec3, 3> origins{{{0.25, -0.2, 0.5},
        {-2.1, 1.2, 1.0}, {3.0, -2.0, 0.0}}};
    size_t hits = 0;
    size_t misses = 0;
    for (Vec3 origin : origins) {
        for (int y = -10; y <= 10; ++y) {
            for (int x = -12; x <= 12; ++x) {
                Ray ray{origin, normalized({x * 0.06 + 0.013,
                    y * 0.055 - 0.017, -1.0})};
                compare_bvh_hit(bvh, triangles, ray, hits, misses);
            }
        }
    }
    require(hits > 100 && misses > 100, "3D BVH hit and miss coverage");
}
}

int main() {
    try {
        triangle_cases();
        box_cases();
        bvh_cases();
        bvh_3d_cases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
