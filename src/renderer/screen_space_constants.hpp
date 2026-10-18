#pragma once

#include "config/config.hpp"

#include <cmath>
#include <cstdint>
#include <optional>

namespace neuralfx {
enum class ColorEncoding { Srgb, Linear };

struct alignas(16) ScreenSpaceConstants {
    float width;
    float height;
    float inverse_width;
    float inverse_height;
    float tan_half_fov;
    float aspect;
    float near_plane;
    float far_plane;
    float max_distance;
    float thickness;
    float occlusion_radius;
    float reflection_strength;
    float occlusion_strength;
    float bounce_strength;
    std::uint32_t depth_convention;
    std::uint32_t debug_view;
    std::uint32_t decode_input;
    std::uint32_t padding[3];
};

static_assert(sizeof(ScreenSpaceConstants) == 80);

inline std::optional<ScreenSpaceConstants> make_screen_space_constants(
    const RaytracingSettings& settings, unsigned width, unsigned height,
    ColorEncoding input) noexcept {
    if (width == 0 || height == 0) return std::nullopt;
    constexpr float degrees_to_half_radians = 3.14159265358979f / 360.0f;
    float columns = static_cast<float>(width);
    float rows = static_cast<float>(height);
    return ScreenSpaceConstants{
        columns,
        rows,
        1.0f / columns,
        1.0f / rows,
        std::tan(settings.vertical_fov * degrees_to_half_radians),
        columns / rows,
        settings.near_plane,
        settings.far_plane,
        settings.max_distance,
        settings.thickness,
        settings.occlusion_radius,
        settings.reflection_strength,
        settings.occlusion_strength,
        settings.bounce_strength,
        static_cast<std::uint32_t>(settings.depth),
        static_cast<std::uint32_t>(settings.debug_view),
        static_cast<std::uint32_t>(input == ColorEncoding::Srgb),
        {0, 0, 0}
    };
}
}
