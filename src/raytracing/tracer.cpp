#include "raytracing/tracer.hpp"

#include <algorithm>
#include <limits>

namespace neuralfx::raytracing {
namespace {
Vec3 background(const Ray& ray, const Settings& settings) noexcept {
    double blend = std::clamp((normalized(ray.direction).y + 1.0) * 0.5, 0.0, 1.0);
    return settings.background_bottom * (1.0 - blend) + settings.background_top * blend;
}

Vec3 direct_light(const SceneQuery& scene, const Hit& hit,
    const Material& material, const Settings& settings) noexcept {
    Vec3 result = multiply(material.base_color, settings.ambient);
    for (const DirectionalLight& light : scene.lights()) {
        if (light.intensity <= 0.0) continue;
        Vec3 direction = light.to_light();
        double cosine = dot(hit.normal, direction);
        if (cosine <= 0.0) continue;
        Ray shadow{hit.position + hit.normal * settings.epsilon, direction};
        if (scene.occluded(shadow, settings.epsilon, std::numeric_limits<double>::infinity()))
            continue;
        result = result + multiply(material.base_color, light.color) * (light.intensity * cosine);
    }
    return result;
}

Vec3 trace_bounce(const SceneQuery& scene, const Ray& ray,
    const Settings& settings, size_t depth) {
    auto hit = scene.intersect(ray, settings.epsilon, std::numeric_limits<double>::infinity());
    if (!hit) return background(ray, settings);
    const Material* material = scene.material(hit->material);
    if (!material) return {};
    Vec3 local = direct_light(scene, *hit, *material, settings);
    double reflectivity = std::clamp(material->reflectivity, 0.0, 1.0);
    if (reflectivity == 0.0 || depth >= settings.max_bounces) return local;
    Vec3 reflected = normalized(ray.direction - hit->normal * (2.0 * dot(ray.direction, hit->normal)));
    Ray bounce{hit->position + hit->normal * settings.epsilon, reflected};
    Vec3 reflection = trace_bounce(scene, bounce, settings, depth + 1);
    return local * (1.0 - reflectivity) + reflection * reflectivity;
}
}

Vec3 trace(const SceneQuery& scene, const Ray& ray, const Settings& settings) {
    return trace_bounce(scene, ray, settings, 0);
}
}
