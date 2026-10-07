#include "raytracing/scene.hpp"

#include <stdexcept>
#include <utility>

namespace neuralfx::raytracing {
namespace {
std::vector<Triangle> validated_triangles(std::vector<Triangle> triangles,
    size_t material_count) {
    for (const Triangle& triangle : triangles) {
        if (triangle.material >= material_count)
            throw std::invalid_argument("Triangle material index is out of range");
    }
    return triangles;
}
}

Scene::Scene(std::vector<Triangle> triangles, std::vector<Material> materials,
    std::vector<DirectionalLight> lights)
    : geometry_(validated_triangles(std::move(triangles), materials.size())),
      materials_(std::move(materials)), lights_(std::move(lights)) {}

void Scene::rebuild(std::vector<Triangle> triangles) {
    geometry_.rebuild(validated_triangles(std::move(triangles), materials_.size()));
}

std::optional<Hit> Scene::intersect(const Ray& ray, double minimum, double maximum) const noexcept {
    return geometry_.intersect(ray, minimum, maximum);
}

bool Scene::occluded(const Ray& ray, double minimum, double maximum) const noexcept {
    return geometry_.occluded(ray, minimum, maximum);
}

const Material* Scene::material(size_t index) const noexcept {
    return index < materials_.size() ? &materials_[index] : nullptr;
}

std::span<const DirectionalLight> Scene::lights() const noexcept { return lights_; }
}
