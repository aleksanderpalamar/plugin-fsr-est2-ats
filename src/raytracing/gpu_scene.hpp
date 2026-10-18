#pragma once

#include "raytracing/scene.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace neuralfx::raytracing {
using Float3 = std::array<float, 3>;
inline constexpr size_t gpu_traversal_stack_size = 40;

struct GpuNode {
    Float3 bounds_min;
    std::uint32_t first_primitive;
    Float3 bounds_max;
    std::uint32_t primitive_count;
    std::uint32_t left_child;
    std::uint32_t right_child;
    std::uint32_t padding[2];
};

struct GpuTriangle {
    Float3 a;
    std::uint32_t material;
    Float3 b;
    std::uint32_t has_normals;
    Float3 c;
    std::uint32_t padding_c;
    Float3 normal_a;
    std::uint32_t padding_normal_a;
    Float3 normal_b;
    std::uint32_t padding_normal_b;
    Float3 normal_c;
    std::uint32_t padding_normal_c;
};

struct GpuMaterial {
    Float3 base_color;
    float reflectivity;
};

struct GpuLight {
    Float3 to_light;
    float intensity;
    Float3 color;
    float padding;
};

static_assert(sizeof(GpuNode) == 48);
static_assert(sizeof(GpuTriangle) == 96);
static_assert(sizeof(GpuMaterial) == 16);
static_assert(sizeof(GpuLight) == 32);

struct GpuSceneData {
    std::vector<GpuNode> nodes;
    std::vector<GpuTriangle> triangles;
    std::vector<GpuMaterial> materials;
    std::vector<GpuLight> lights;
};

struct GpuSceneCounts {
    std::uint32_t nodes = 0;
    std::uint32_t lights = 0;
    std::uint32_t materials = 0;
};

Float3 to_float3(Vec3 value) noexcept;
GpuSceneData pack_scene(const Scene& scene);
GpuSceneCounts scene_counts(const GpuSceneData& scene) noexcept;
}
