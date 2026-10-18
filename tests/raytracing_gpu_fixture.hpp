#pragma once

#include "raytracing/gpu_tracer.hpp"
#include "raytracing/scene.hpp"
#include "d3d11_test_device.hpp"

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

namespace neuralfx::testing {
using namespace neuralfx::raytracing;

inline void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

inline void require_near(double actual, double expected, double tolerance, const std::string& message) {
    require(std::abs(actual - expected) <= tolerance, message + " (measured " +
        std::to_string(actual) + ", expected " + std::to_string(expected) + ")");
}

struct TracerFixture {
    TestDevice gpu;
    GpuRaytracer tracer;
};

inline Triangle surface(double x, double z, double radius, size_t material) {
    return {std::array<Vec3, 3>{{{x - radius, -radius, z},
        {x + radius, -radius, z}, {x, radius, z}}}, std::nullopt, material};
}

inline Triangle marker(double x, double y, double z, double radius, size_t material) {
    return {std::array<Vec3, 3>{{{x - radius, y - radius, z},
        {x + radius, y - radius, z}, {x, y + radius, z}}}, std::nullopt, material};
}

inline Settings dark_settings() {
    Settings settings;
    settings.ambient = {0, 0, 0};
    settings.background_bottom = {0, 0, 0};
    settings.background_top = {0, 0, 0};
    return settings;
}

inline std::optional<Image> render_packed(TracerFixture& fixture, const GpuSceneData& scene,
    const Camera& camera, int width, int height, const Settings& settings) {
    require(SUCCEEDED(fixture.tracer.upload(scene)), "scene uploaded");
    return fixture.tracer.render(fixture.gpu.context.Get(), camera, width, height, settings);
}

inline std::optional<Image> render(TracerFixture& fixture, const Scene& scene,
    const Camera& camera, int width, int height, const Settings& settings) {
    return render_packed(fixture, pack_scene(scene), camera, width, height, settings);
}

inline Vec3 trace_forward(TracerFixture& fixture, const Scene& scene, const Settings& settings) {
    auto camera = Camera::look_at({0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 60.0);
    require(camera.has_value(), "forward camera");
    auto image = render(fixture, scene, *camera, 1, 1, settings);
    require(image.has_value(), "forward ray rendered");
    return image->at(0, 0);
}
}
