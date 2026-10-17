#include "raytracing/gpu_scene.hpp"

#include <algorithm>
#include <variant>

namespace neuralfx::raytracing {
namespace {
std::uint32_t index32(size_t value) noexcept {
    return static_cast<std::uint32_t>(value);
}

GpuNode pack_node(const Bvh::Node& node) noexcept {
    GpuNode packed{to_float3(node.bounds.min), 0, to_float3(node.bounds.max), 0, 0, 0, {0, 0}};
    if (const auto* leaf = std::get_if<Bvh::Leaf>(&node.content)) {
        packed.first_primitive = index32(leaf->first);
        packed.primitive_count = index32(leaf->count);
    }
    if (const auto* branch = std::get_if<Bvh::Branch>(&node.content)) {
        packed.left_child = index32(branch->left);
        packed.right_child = index32(branch->right);
    }
    return packed;
}

GpuTriangle pack_triangle(const Triangle& triangle) noexcept {
    GpuTriangle packed{};
    packed.a = to_float3(triangle.positions[0]);
    packed.b = to_float3(triangle.positions[1]);
    packed.c = to_float3(triangle.positions[2]);
    packed.material = index32(triangle.material);
    if (!triangle.normals) return packed;
    packed.has_normals = 1;
    packed.normal_a = to_float3((*triangle.normals)[0]);
    packed.normal_b = to_float3((*triangle.normals)[1]);
    packed.normal_c = to_float3((*triangle.normals)[2]);
    return packed;
}

GpuMaterial pack_material(const Material& material) noexcept {
    return {to_float3(material.base_color),
        static_cast<float>(std::clamp(material.reflectivity, 0.0, 1.0))};
}

GpuLight pack_light(const DirectionalLight& light) noexcept {
    return {to_float3(light.to_light()), static_cast<float>(light.intensity), to_float3(light.color), 0.0f};
}
}

Float3 to_float3(Vec3 value) noexcept {
    return {static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z)};
}

GpuSceneData pack_scene(const Scene& scene) {
    const Bvh& geometry = scene.geometry();
    GpuSceneData packed;
    packed.nodes.reserve(geometry.nodes().size());
    for (const Bvh::Node& node : geometry.nodes()) packed.nodes.push_back(pack_node(node));
    packed.triangles.reserve(geometry.primitive_order().size());
    for (size_t primitive : geometry.primitive_order())
        packed.triangles.push_back(pack_triangle(geometry.triangles()[primitive]));
    for (const Material& material : scene.materials()) packed.materials.push_back(pack_material(material));
    for (const DirectionalLight& light : scene.lights()) packed.lights.push_back(pack_light(light));
    return packed;
}

GpuSceneCounts scene_counts(const GpuSceneData& scene) noexcept {
    return {index32(scene.nodes.size()), index32(scene.lights.size()), index32(scene.materials.size())};
}
}
