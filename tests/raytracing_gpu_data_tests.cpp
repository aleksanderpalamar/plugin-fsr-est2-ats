#include "raytracing/camera.hpp"
#include "raytracing/gpu_constants.hpp"
#include "raytracing/gpu_scene.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

using namespace neuralfx::raytracing;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(Vec3 actual, Vec3 expected) {
    return length(actual - expected) < 1e-9;
}

void camera_frame_cases() {
    auto camera = Camera::look_at({1, 2, 3}, {1, 2, 2}, {0, 1, 0}, 90.0);
    require(camera.has_value(), "valid camera");
    auto frame = camera->frame(4, 2);
    require(frame.has_value(), "frame for a valid size");
    require(near(frame->position, {1, 2, 3}), "frame position");
    require(near(frame->forward, {0, 0, -1}), "frame forward");
    require(near(frame->horizontal, {2, 0, 0}), "horizontal spans aspect times half-angle tangent");
    require(near(frame->vertical, {0, 1, 0}), "vertical spans half-angle tangent");
    auto narrow = Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 60.0)->frame(2, 2);
    require(narrow && std::abs(length(narrow->vertical) - std::tan(std::numbers::pi / 6.0)) < 1e-9,
        "vertical field of view sets the vertical span");
    require(!camera->frame(0, 2), "zero width rejected");
    require(!camera->frame(2, -1), "negative height rejected");
}

Triangle surface(double x, double z, size_t material = 0) {
    return {std::array<Vec3, 3>{{{x - 1.0, -1.0, z}, {x + 1.0, -1.0, z}, {x, 1.0, z}}},
        std::nullopt, material};
}

bool same(Float3 actual, Vec3 expected) {
    return std::abs(actual[0] - expected.x) < 1e-6 && std::abs(actual[1] - expected.y) < 1e-6 &&
        std::abs(actual[2] - expected.z) < 1e-6;
}

void node_cases(const GpuSceneData& packed, size_t triangle_count) {
    std::vector<unsigned> covered(triangle_count, 0);
    for (size_t index = 0; index < packed.nodes.size(); ++index) {
        const GpuNode& node = packed.nodes[index];
        if (node.primitive_count == 0) {
            require(node.left_child > index && node.right_child > index, "children follow parent");
            require(node.left_child < packed.nodes.size() && node.right_child < packed.nodes.size(),
                "children inside node array");
            continue;
        }
        require(node.first_primitive + node.primitive_count <= triangle_count, "leaf range inside triangles");
        for (unsigned offset = 0; offset < node.primitive_count; ++offset)
            ++covered[node.first_primitive + offset];
    }
    require(std::all_of(covered.begin(), covered.end(), [](unsigned count) { return count == 1; }),
        "leaves cover every packed triangle once");
}

void pack_cases() {
    std::vector<Triangle> triangles;
    for (int index = -4; index <= 4; ++index)
        triangles.push_back(surface(index * 3.0, -3.0 - (index & 1), static_cast<size_t>(index + 4) % 2));
    triangles[0].normals = std::array<Vec3, 3>{{{0, 0, 1}, {0, 1, 1}, {1, 0, 0}}};
    Scene scene(triangles, {{{1, 0.5, 0.25}, 1.5}, {{0, 1, 0}, -0.2}},
        {{{0, 3, 4}, {0.5, 0.6, 0.7}, 2.0}});
    GpuSceneData packed = pack_scene(scene);
    require(packed.nodes.size() == scene.bvh_node_count() && packed.nodes.size() > 1, "nodes packed");
    require(packed.triangles.size() == triangles.size(), "triangles packed");
    node_cases(packed, triangles.size());
    for (const Triangle& original : triangles) {
        auto match = std::find_if(packed.triangles.begin(), packed.triangles.end(),
            [&](const GpuTriangle& candidate) { return same(candidate.a, original.positions[0]); });
        require(match != packed.triangles.end(), "original triangle packed");
        require(same(match->b, original.positions[1]) && same(match->c, original.positions[2]),
            "triangle vertices packed");
        require(match->material == original.material, "triangle material packed");
        require(match->has_normals == (original.normals ? 1u : 0u), "normal flag packed");
        if (original.normals) require(same(match->normal_b, (*original.normals)[1]), "normals packed");
    }
    require(same(packed.nodes[0].bounds_min, {-13, -1, -4}) && same(packed.nodes[0].bounds_max, {13, 1, -3}),
        "root bounds packed");
    require(packed.materials.size() == 2 && same(packed.materials[0].base_color, {1, 0.5, 0.25}),
        "material color packed");
    require(packed.materials[0].reflectivity == 1.0f && packed.materials[1].reflectivity == 0.0f,
        "reflectivity clamped");
    require(packed.lights.size() == 1 && same(packed.lights[0].to_light, {0, 0.6, 0.8}), "light direction");
    require(same(packed.lights[0].color, {0.5, 0.6, 0.7}) && packed.lights[0].intensity == 2.0f,
        "light color and intensity");
    GpuSceneData empty = pack_scene(Scene({}, {}, {}));
    require(empty.nodes.empty() && empty.triangles.empty(), "empty scene packs no geometry");
}

size_t subtree_depth(const GpuSceneData& packed, std::uint32_t index) {
    const GpuNode& node = packed.nodes[index];
    if (node.primitive_count > 0) return 1;
    return 1 + std::max(subtree_depth(packed, node.left_child), subtree_depth(packed, node.right_child));
}

void traversal_stack_cases() {
    std::vector<Triangle> triangles;
    for (std::uint64_t index = 0; index < 20000; ++index) {
        double x = static_cast<double>(index * 7919 % 1000) * 0.01;
        double y = static_cast<double>(index * 104729 % 1000) * 0.01;
        double z = -1.0 - static_cast<double>(index * 1299709 % 1000) * 0.01;
        triangles.push_back({{{{x, y, z}, {x + 0.01, y, z}, {x, y + 0.01, z}}}, std::nullopt, 0});
    }
    GpuSceneData packed = pack_scene(Scene(triangles, {{{1, 1, 1}, 0.0}}, {}));
    require(subtree_depth(packed, 0) + 1 <= gpu_traversal_stack_size,
        "BVH depth fits the GPU traversal stack");
}

void constants_cases() {
    auto camera = Camera::look_at({1, 2, 3}, {1, 2, 2}, {0, 1, 0}, 90.0);
    require(camera.has_value(), "constants camera");
    GpuSceneData scene;
    scene.nodes.resize(3);
    scene.lights.resize(2);
    scene.materials.resize(4);
    Settings settings;
    settings.ambient = {0.1, 0.2, 0.3};
    settings.background_bottom = {0.4, 0.5, 0.6};
    settings.background_top = {0.7, 0.8, 0.9};
    settings.epsilon = 0.001;
    auto constants = make_trace_constants(*camera, 4, 2, settings, scene_counts(scene));
    require(constants.has_value(), "constants built");
    require(constants->width == 4 && constants->height == 2, "image size");
    require(same(constants->camera_position, {1, 2, 3}) && same(constants->camera_forward, {0, 0, -1}),
        "camera origin and forward");
    require(same(constants->camera_horizontal, {2, 0, 0}) && same(constants->camera_vertical, {0, 1, 0}),
        "camera spans");
    require(constants->node_count == 3 && constants->light_count == 2 && constants->material_count == 4,
        "scene counts");
    require(same(constants->ambient, {0.1, 0.2, 0.3}) && std::abs(constants->epsilon - 0.001f) < 1e-9f,
        "ambient and epsilon");
    require(same(constants->background_bottom, {0.4, 0.5, 0.6}) &&
        same(constants->background_top, {0.7, 0.8, 0.9}), "background gradient");
    require(constants->max_bounces == 3, "bounce limit copied");
    settings.max_bounces = 1000;
    require(make_trace_constants(*camera, 4, 2, settings, scene_counts(scene))->max_bounces == max_gpu_bounces,
        "bounce limit capped for the GPU loop");
    require(!make_trace_constants(*camera, 0, 2, settings, scene_counts(scene)), "invalid size rejected");
}
}

int main() {
    try {
        camera_frame_cases();
        pack_cases();
        constants_cases();
        traversal_stack_cases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
