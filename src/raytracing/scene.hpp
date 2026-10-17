#pragma once

#include "raytracing/bvh.hpp"

#include <span>
#include <vector>

namespace neuralfx::raytracing {
struct Material {
    Vec3 base_color{1, 1, 1};
    double reflectivity = 0.0;
};

class DirectionalLight {
public:
    Vec3 color;
    double intensity;

    DirectionalLight(Vec3 direction = {0, 1, 0}, Vec3 color = {1, 1, 1},
        double intensity = 1.0) noexcept
        : color(color), intensity(intensity), to_light_(normalized(direction)) {}

    Vec3 to_light() const noexcept { return to_light_; }
    void set_direction(Vec3 direction) noexcept { to_light_ = normalized(direction); }

private:
    Vec3 to_light_;
};

class Scene {
public:
    Scene(std::vector<Triangle> triangles, std::vector<Material> materials,
        std::vector<DirectionalLight> lights);
    void rebuild(std::vector<Triangle> triangles);
    size_t bvh_node_count() const noexcept { return geometry_.node_count(); }
    const Bvh& geometry() const noexcept { return geometry_; }
    std::span<const Material> materials() const noexcept { return materials_; }
    std::span<const DirectionalLight> lights() const noexcept { return lights_; }

private:
    Bvh geometry_;
    std::vector<Material> materials_;
    std::vector<DirectionalLight> lights_;
};
}
