#pragma once

#include "raytracing/camera.hpp"
#include "raytracing/scene.hpp"

#include <cstddef>

namespace neuralfx::raytracing {
struct Settings {
    size_t max_bounces = 3;
    double epsilon = ray_epsilon;
    Vec3 ambient{0.08, 0.08, 0.08};
    Vec3 background_bottom{0.65, 0.75, 0.95};
    Vec3 background_top{0.1, 0.25, 0.55};
};

Vec3 trace(const SceneQuery& scene, const Ray& ray, const Settings& settings);
}
