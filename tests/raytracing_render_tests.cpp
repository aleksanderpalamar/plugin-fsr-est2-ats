#include "raytracing/image.hpp"
#include "raytracing/scene.hpp"
#include "raytracing/tracer.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace neuralfx::raytracing;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

Triangle surface(double x, double z, double radius, size_t material) {
    return {std::array<Vec3, 3>{{{x - radius, -radius, z},
        {x + radius, -radius, z}, {x, radius, z}}}, std::nullopt, material};
}

void camera_cases() {
    auto camera = Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 90.0);
    require(camera.has_value(), "valid camera");
    require(!Camera::look_at({0, 0, 0}, {0, 0, 0}, {0, 1, 0}, 90.0), "zero view direction");
    require(!Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 0, -1}, 90.0), "parallel up vector");
    require(!Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 180.0), "invalid field of view");
    Ray center = camera->ray_for_pixel(1, 1, 3, 3);
    require(std::abs(center.direction.x) < 1e-10, "camera center x");
    require(std::abs(center.direction.y) < 1e-10, "camera center y");
    require(center.direction.z < -0.99, "camera center forward");
    Ray right = camera->ray_for_pixel(2, 1, 3, 3);
    require(right.direction.x > 0.0, "camera right ray");
    Ray wide = camera->ray_for_pixel(3, 1, 4, 2);
    require(wide.direction.x > right.direction.x, "camera aspect ratio");
}

void light_and_shadow_cases() {
    Material red{{1, 0, 0}, 0.0};
    DirectionalLight light{{1, 0, 1}, {1, 1, 1}, 1.0};
    Settings settings;
    settings.ambient = {0.1, 0.1, 0.1};
    Ray primary{{0, 0, 0}, {0, 0, -1}};
    Scene lit({surface(0, -3, 1, 0)}, {red}, {light});
    Vec3 lit_color = trace(lit, primary, settings);
    require(lit_color.x > 0.7 && lit_color.y == 0.0, "Lambert lighting");
    Scene shadowed({surface(0, -3, 1, 0), surface(0.5, -2.5, 0.2, 0)}, {red}, {light});
    Vec3 shadow_color = trace(shadowed, primary, settings);
    require(shadow_color.x < 0.2, "shadow ray blocks direct light");
    require(shadow_color.x > 0.0, "ambient light remains in shadow");
}

void directional_light_cases() {
    DirectionalLight light{{0, 3, 4}, {1, 1, 1}, 1.0};
    require(std::abs(length(light.to_light) - 1.0) < 1e-12,
        "directional light normalized at construction");
}

void reflection_cases() {
    Settings settings;
    settings.ambient = {0, 0, 0};
    settings.background_bottom = {0.1, 0.2, 0.7};
    settings.background_top = settings.background_bottom;
    Scene mirror({surface(0, -3, 1, 0)}, {{{1, 0, 0}, 1.0}}, {});
    Ray primary{{0, 0, 0}, {0, 0, -1}};
    Vec3 reflected = trace(mirror, primary, settings);
    require(std::abs(reflected.z - 0.7) < 1e-8, "mirror reflects background");
    Scene reflected_object({surface(0, -3, 1, 0), surface(0, 2, 1, 1)},
        {{{1, 1, 1}, 1.0}, {{1, 0, 0}, 0.0}}, {{{0, 0, -1}, {1, 1, 1}, 1.0}});
    Settings object_settings = settings;
    object_settings.ambient = {1, 1, 1};
    Vec3 object_color = trace(reflected_object, primary, object_settings);
    require(object_color.x > 0.9 && object_color.z == 0.0, "mirror reflects geometry");
    settings.max_bounces = 0;
    require(trace(mirror, primary, settings).z == 0.0, "bounce limit");
    Scene empty({}, {}, {});
    require(std::abs(trace(empty, primary, settings).z - 0.7) < 1e-8, "background on miss");
}

void material_cases() {
    bool rejected = false;
    try {
        Scene invalid({surface(0, -3, 1, 1)}, {{{1, 0, 0}, 0.0}}, {});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "invalid material rejected during scene construction");

    Scene scene({surface(0, -3, 1, 0)}, {{{1, 0, 0}, 0.0}}, {});
    rejected = false;
    try {
        scene.rebuild({surface(0, -4, 1, 1)});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "invalid material rejected during rebuild");
    auto hit = scene.intersect({{0, 0, 0}, {0, 0, -1}}, ray_epsilon, 10.0);
    require(hit && hit->material == 0, "rejected rebuild preserves geometry");
}

void image_cases() {
    auto camera = Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 60.0);
    require(camera.has_value(), "render camera");
    Scene scene({surface(0, -3, 2, 0)}, {{{1, 0, 0}, 0.0}},
        {{{0, 0, 1}, {1, 1, 1}, 1.0}});
    auto image = render(scene, *camera, 32, 16, {});
    require(image.has_value(), "image rendered");
    require(image->pixels.size() == 32 * 16, "framebuffer size");
    require(image->at(16, 8).x > image->at(0, 0).x, "scene visible in framebuffer");
    require(!render(scene, *camera, 0, 16, {}), "invalid image dimensions");
    std::ostringstream stream;
    require(write_ppm(stream, *image), "PPM output");
    require(stream.str().starts_with("P6\n32 16\n255\n"), "PPM header");
}

void multi_triangle_image_cases() {
    auto camera = Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 60.0);
    require(camera.has_value(), "multi-triangle camera");
    Scene scene({surface(-2, -3, 0.4, 0), surface(-1, -3, 0.4, 1),
        surface(0, -3, 0.4, 2), surface(1, -3, 0.4, 3),
        surface(2, -3, 0.4, 4)},
        {{{1, 0, 0}, 0}, {{0, 1, 0}, 0}, {{0, 0, 1}, 0},
            {{1, 1, 0}, 0}, {{1, 0, 1}, 0}},
        {{{0, 0, 2}, {1, 1, 1}, 1.0}});
    require(scene.bvh_node_count() > 1, "multi-triangle BVH branches");
    Settings settings;
    settings.ambient = {0, 0, 0};
    auto image = render(scene, *camera, 80, 40, settings);
    require(image.has_value(), "multi-triangle image rendered");
    const std::array<int, 5> pixels{16, 28, 40, 52, 64};
    const std::array<Vec3, 5> colors{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1},
        {1, 1, 0}, {1, 0, 1}}};
    for (size_t index = 0; index < pixels.size(); ++index)
        require(length(image->at(pixels[index], 20) - colors[index]) < 1e-8,
            "distinct triangles visible in image");
}
}

int main() {
    try {
        camera_cases();
        light_and_shadow_cases();
        directional_light_cases();
        reflection_cases();
        material_cases();
        image_cases();
        multi_triangle_image_cases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
