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

class SceneQuery {
public:
    virtual ~SceneQuery() = default;
    virtual std::optional<Hit> intersect(const Ray& ray, double minimum,
        double maximum) const noexcept = 0;
    virtual bool occluded(const Ray& ray, double minimum, double maximum) const noexcept = 0;
    virtual const Material* material(size_t index) const noexcept = 0;
    virtual std::span<const DirectionalLight> lights() const noexcept = 0;
};

class Scene final : public SceneQuery {
public:
    Scene(std::vector<Triangle> triangles, std::vector<Material> materials,
        std::vector<DirectionalLight> lights);
    void rebuild(std::vector<Triangle> triangles);
    std::optional<Hit> intersect(const Ray& ray, double minimum,
        double maximum) const noexcept override;
    bool occluded(const Ray& ray, double minimum, double maximum) const noexcept override;
    const Material* material(size_t index) const noexcept override;
    std::span<const DirectionalLight> lights() const noexcept override;
    size_t bvh_node_count() const noexcept { return geometry_.node_count(); }

private:
    Bvh geometry_;
    std::vector<Material> materials_;
    std::vector<DirectionalLight> lights_;
};
}
