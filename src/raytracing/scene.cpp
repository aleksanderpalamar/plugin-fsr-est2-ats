#include "raytracing/scene.hpp"

#include <utility>

namespace neuralfx::raytracing {
Scene::Scene(std::vector<Triangle> triangles, std::vector<Material> materials,
    std::vector<DirectionalLight> lights)
    : geometry_(std::move(triangles)), materials_(std::move(materials)), lights_(std::move(lights)) {}

void Scene::rebuild(std::vector<Triangle> triangles) { geometry_.rebuild(std::move(triangles)); }

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
