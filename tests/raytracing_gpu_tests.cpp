#include "common/logger.hpp"
#include "raytracing_gpu_fixture.hpp"

#include <iostream>
#include <vector>

using namespace neuralfx::testing;

namespace {
void camera_cases(TracerFixture& fixture) {
    auto camera = Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 90.0);
    require(camera.has_value(), "camera");
    Scene right({marker(2.0, 0.0, -3.0, 0.3, 0)}, {{{1, 0, 0}, 0.0}}, {{{0, 0, 1}, {1, 1, 1}, 1.0}});
    auto image = render(fixture, right, *camera, 3, 3, dark_settings());
    require(image && image->width == 3 && image->height == 3, "camera image rendered");
    require(image->at(2, 1).x > 0.9, "right pixel ray reaches geometry on the right");
    require(image->at(1, 1).x == 0.0 && image->at(0, 1).x == 0.0, "other pixels miss");
    Scene above({marker(0.0, 2.0, -3.0, 0.3, 0)}, {{{1, 0, 0}, 0.0}}, {{{0, 0, 1}, {1, 1, 1}, 1.0}});
    image = render(fixture, above, *camera, 3, 3, dark_settings());
    require(image && image->at(1, 0).x > 0.9 && image->at(1, 2).x == 0.0, "top row looks up");
}

void light_and_shadow_cases(TracerFixture& fixture) {
    Material red{{1, 0, 0}, 0.0};
    DirectionalLight light{{1, 0, 1}, {1, 1, 1}, 1.0};
    Settings settings;
    settings.ambient = {0.1, 0.1, 0.1};
    Vec3 lit = trace_forward(fixture, Scene({surface(0, -3, 1, 0)}, {red}, {light}), settings);
    require(lit.x > 0.7 && lit.y == 0.0, "Lambert lighting");
    Vec3 shadow = trace_forward(fixture,
        Scene({surface(0, -3, 1, 0), surface(0.5, -2.5, 0.2, 0)}, {red}, {light}), settings);
    require(shadow.x < 0.2, "shadow ray blocks direct light");
    require(shadow.x > 0.0, "ambient light remains in shadow");
}

void directional_light_cases(TracerFixture& fixture) {
    DirectionalLight changed;
    changed.set_direction({0, 0, 2});
    changed.intensity = 0.4;
    Vec3 color = trace_forward(fixture,
        Scene({surface(0, -3, 1, 0)}, {{{1, 1, 1}, 0.0}}, {changed}), dark_settings());
    require_near(color.x, 0.4, 1e-5, "changed light direction preserves diffuse intensity");
}

void reflection_cases(TracerFixture& fixture) {
    Settings settings = dark_settings();
    settings.background_bottom = {0.1, 0.2, 0.7};
    settings.background_top = settings.background_bottom;
    Scene mirror({surface(0, -3, 1, 0)}, {{{1, 0, 0}, 1.0}}, {});
    require_near(trace_forward(fixture, mirror, settings).z, 0.7, 1e-5, "mirror reflects background");
    Scene reflected({surface(0, -3, 1, 0), surface(0, 2, 1, 1)},
        {{{1, 1, 1}, 1.0}, {{1, 0, 0}, 0.0}}, {{{0, 0, -1}, {1, 1, 1}, 1.0}});
    Settings object_settings = settings;
    object_settings.ambient = {1, 1, 1};
    Vec3 object = trace_forward(fixture, reflected, object_settings);
    require(object.x > 0.9 && object.z == 0.0, "mirror reflects geometry");
    settings.max_bounces = 0;
    require(trace_forward(fixture, mirror, settings).z == 0.0, "bounce limit");
    require_near(trace_forward(fixture, Scene({}, {}, {}), settings).z, 0.7, 1e-5, "background on miss");
}

void image_cases(TracerFixture& fixture) {
    auto camera = Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 60.0);
    Scene scene({surface(0, -3, 2, 0)}, {{{1, 0, 0}, 0.0}}, {{{0, 0, 1}, {1, 1, 1}, 1.0}});
    auto image = render(fixture, scene, *camera, 32, 16, {});
    require(image && image->pixels.size() == 32 * 16, "framebuffer size");
    require(image->at(16, 8).x > image->at(0, 0).x, "scene visible in framebuffer");
    require(!render(fixture, scene, *camera, 0, 16, {}), "invalid image dimensions");
}

void multi_triangle_cases(TracerFixture& fixture) {
    auto camera = Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 60.0);
    Scene scene({surface(-2, -3, 0.4, 0), surface(-1, -3, 0.4, 1), surface(0, -3, 0.4, 2),
        surface(1, -3, 0.4, 3), surface(2, -3, 0.4, 4)},
        {{{1, 0, 0}, 0}, {{0, 1, 0}, 0}, {{0, 0, 1}, 0}, {{1, 1, 0}, 0}, {{1, 0, 1}, 0}},
        {{{0, 0, 2}, {1, 1, 1}, 1.0}});
    require(scene.bvh_node_count() > 1, "multi-triangle BVH branches");
    auto image = render(fixture, scene, *camera, 80, 40, dark_settings());
    require(image.has_value(), "multi-triangle image rendered");
    const std::array<int, 5> pixels{16, 28, 40, 52, 64};
    const std::array<Vec3, 5> colors{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 0}, {1, 0, 1}}};
    for (size_t index = 0; index < pixels.size(); ++index)
        require(length(image->at(pixels[index], 20) - colors[index]) < 1e-4,
            "distinct triangles visible in image");
}

Scene layered_scene() {
    std::vector<Triangle> triangles;
    for (int layer = 0; layer < 3; ++layer)
        for (int y = -3; y <= 3; ++y)
            for (int x = -4; x <= 4; ++x) {
                double cx = x * 1.4;
                double cy = y * 1.4;
                double cz = -4.0 - layer * 1.6;
                triangles.push_back({{{{cx - 0.45, cy - 0.40, cz - 0.10}, {cx + 0.45, cy - 0.35, cz + 0.15},
                    {cx + 0.05, cy + 0.50, cz - 0.05}}}, std::nullopt, static_cast<size_t>(layer)});
            }
    return Scene(triangles, {{{1, 0.2, 0.2}, 0.0}, {{0.2, 1, 0.2}, 0.3}, {{0.2, 0.2, 1}, 0.0}},
        {{{0.3, 0.4, 1.0}, {1, 1, 1}, 1.0}});
}

void bvh_agreement_cases(TracerFixture& fixture) {
    Scene scene = layered_scene();
    GpuSceneData accelerated = pack_scene(scene);
    GpuSceneData brute_force = accelerated;
    GpuNode root = accelerated.nodes.front();
    root.first_primitive = 0;
    root.primitive_count = static_cast<std::uint32_t>(accelerated.triangles.size());
    brute_force.nodes = {root};
    auto camera = Camera::look_at({0.25, -0.2, 0.5}, {0.25, -0.2, -1.0}, {0, 1, 0}, 70.0);
    auto fast = render_packed(fixture, accelerated, *camera, 64, 48, dark_settings());
    auto reference = render_packed(fixture, brute_force, *camera, 64, 48, dark_settings());
    require(fast && reference, "agreement images rendered");
    size_t hits = 0;
    for (size_t index = 0; index < fast->pixels.size(); ++index) {
        require(length(fast->pixels[index] - reference->pixels[index]) < 1e-5,
            "BVH traversal matches brute force");
        if (length(reference->pixels[index]) > 0.0) ++hits;
    }
    require(hits > 300 && hits < fast->pixels.size() - 300, "agreement covers hits and misses");
}
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) {
        std::cerr << "D3D11 device unavailable\n";
        return 1;
    }
    TracerFixture fixture{*gpu, {}};
    auto shaders = neuralfx::module_directory() / L"NeuralFX" / L"shaders";
    if (FAILED(fixture.tracer.initialize(gpu->device.Get(), shaders))) {
        std::cerr << "GPU ray tracer initialization failed\n";
        return 2;
    }
    constexpr std::array cases{camera_cases, light_and_shadow_cases, directional_light_cases,
        reflection_cases, image_cases, multi_triangle_cases, bvh_agreement_cases};
    int failures = 0;
    for (auto test_case : cases) {
        try {
            test_case(fixture);
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            ++failures;
        }
    }
    return failures == 0 ? 0 : 3;
}
