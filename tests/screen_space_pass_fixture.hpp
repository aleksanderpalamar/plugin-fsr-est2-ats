#pragma once

#include "renderer/screen_space_pass.hpp"
#include "d3d11_test_device.hpp"
#include "screen_space_scene.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace neuralfx::testing {
inline constexpr UINT width = 160;
inline constexpr UINT height = 120;

inline void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

inline void require_above(float actual, float minimum, const std::string& message) {
    require(actual > minimum, message + " (measured " + std::to_string(actual) + ")");
}

inline void require_near(float actual, float expected, float tolerance, const std::string& message) {
    require(std::abs(actual - expected) <= tolerance, message + " (measured " +
        std::to_string(actual) + ", expected " + std::to_string(expected) + ")");
}

struct Fixture {
    TestDevice gpu;
    ScreenSpacePass pass;
};

inline RaytracingSettings settings_for(const Projection& projection) {
    RaytracingSettings settings;
    settings.vertical_fov = 60.0f;
    settings.near_plane = projection.near_plane;
    settings.far_plane = projection.far_plane;
    settings.depth = projection.depth;
    settings.max_distance = 100.0f;
    settings.reflection_strength = 0.0f;
    settings.occlusion_strength = 0.0f;
    settings.bounce_strength = 0.0f;
    return settings;
}

inline ScreenSpaceConstants constants_for(const RaytracingSettings& settings,
    ColorEncoding encoding = ColorEncoding::Linear) {
    auto constants = make_screen_space_constants(settings, width, height, encoding);
    require(constants.has_value(), "constants built");
    return *constants;
}

inline ComPtr<ID3D11ShaderResourceView> float_color(Fixture& fixture, const SyntheticScene& scene) {
    return create_input(fixture.gpu.device.Get(), width, height, DXGI_FORMAT_R32G32B32A32_FLOAT,
        scene.color.data(), static_cast<UINT>(width * sizeof(Pixel)));
}

inline std::vector<Pixel> run(Fixture& fixture, const SyntheticScene& scene,
    ID3D11ShaderResourceView* color, const ScreenSpaceConstants& constants) {
    auto* device = fixture.gpu.device.Get();
    auto depth = create_input(device, width, height, DXGI_FORMAT_R32_FLOAT,
        scene.device_depth.data(), static_cast<UINT>(width * sizeof(float)));
    auto target = create_float_target(device, width, height);
    require(color && depth && target, "test resources created");
    fixture.pass.render(fixture.gpu.context.Get(),
        {color, depth.Get(), target->view.Get(), width, height}, constants);
    auto pixels = read_pixels(fixture.gpu, target->texture.Get(), width, height);
    require(pixels.has_value(), "output read back");
    return *pixels;
}

inline std::vector<Pixel> run(Fixture& fixture, const SyntheticScene& scene,
    const ScreenSpaceConstants& constants) {
    auto color = float_color(fixture, scene);
    return run(fixture, scene, color.Get(), constants);
}

struct Band {
    Surface surface;
    float nearest;
    float farthest;
};

template <typename Measure>
float average(const SyntheticScene& scene, const Band& band, Measure&& measure) {
    float total = 0.0f;
    unsigned count = 0;
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = width / 4; x < width * 3 / 4; ++x) {
            size_t index = scene.index(x, y);
            float distance = scene.view_depth[index];
            if (scene.surfaces[index] != band.surface) continue;
            if (distance < band.nearest || distance > band.farthest) continue;
            total += measure(index);
            ++count;
        }
    }
    require(count > 0, "band sampled");
    return total / static_cast<float>(count);
}

inline float channel_gain(const SyntheticScene& scene, const std::vector<Pixel>& output,
    const Band& band, size_t channel) {
    return average(scene, band, [&](size_t index) {
        return output[index][channel] - scene.color[index][channel];
    });
}
}
