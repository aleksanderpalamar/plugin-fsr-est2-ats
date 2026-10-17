#pragma once

#include "config/config.hpp"
#include "d3d11_test_device.hpp"

#include <cmath>
#include <vector>

namespace neuralfx::testing {
enum class Surface { Ground, Wall, Sky };

struct Projection {
    float near_plane = 0.1f;
    float far_plane = 5000.0f;
    float tan_half_fov = 0.57735f;
    DepthConvention depth = DepthConvention::Standard;
};

struct SyntheticScene {
    UINT width = 0;
    UINT height = 0;
    std::vector<float> device_depth;
    std::vector<float> view_depth;
    std::vector<Pixel> color;
    std::vector<Surface> surfaces;

    size_t index(UINT x, UINT y) const noexcept { return static_cast<size_t>(y) * width + x; }
};

inline constexpr float camera_height = 1.5f;
inline constexpr float wall_distance = 12.0f;
inline constexpr float wall_top = 2.0f;
inline constexpr Pixel ground_color{0.2f, 0.2f, 0.2f, 1.0f};
inline constexpr Pixel wall_color{0.9f, 0.05f, 0.05f, 1.0f};
inline constexpr Pixel sky_color{0.3f, 0.5f, 0.9f, 1.0f};

inline float encode_depth(float view_depth, const Projection& projection) noexcept {
    float n = projection.near_plane;
    float f = projection.far_plane;
    float standard = f * (view_depth - n) / (view_depth * (f - n));
    return projection.depth == DepthConvention::Reversed ? 1.0f - standard : standard;
}

inline float background_depth(const Projection& projection) noexcept {
    return projection.depth == DepthConvention::Reversed ? 0.0f : 1.0f;
}

inline Surface classify(float slope, float& distance) noexcept {
    if (slope < 0.0f) {
        distance = -camera_height / slope;
        if (distance < wall_distance) return Surface::Ground;
    }
    distance = wall_distance;
    return slope * wall_distance <= wall_top ? Surface::Wall : Surface::Sky;
}

inline SyntheticScene make_scene(UINT width, UINT height, const Projection& projection) {
    SyntheticScene scene{width, height};
    size_t count = static_cast<size_t>(width) * height;
    scene.device_depth.resize(count);
    scene.view_depth.resize(count);
    scene.color.resize(count);
    scene.surfaces.resize(count);
    for (UINT y = 0; y < height; ++y) {
        float ndc_y = 1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
        float distance = 0.0f;
        Surface surface = classify(ndc_y * projection.tan_half_fov, distance);
        for (UINT x = 0; x < width; ++x) {
            size_t index = scene.index(x, y);
            scene.surfaces[index] = surface;
            scene.view_depth[index] = surface == Surface::Sky ? projection.far_plane : distance;
            scene.device_depth[index] = surface == Surface::Sky
                ? background_depth(projection) : encode_depth(distance, projection);
            scene.color[index] = surface == Surface::Ground ? ground_color
                : surface == Surface::Wall ? wall_color : sky_color;
        }
    }
    return scene;
}
}
