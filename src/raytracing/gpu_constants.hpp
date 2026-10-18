#pragma once

#include "raytracing/camera.hpp"
#include "raytracing/gpu_scene.hpp"
#include "raytracing/settings.hpp"

#include <cstdint>
#include <optional>

namespace neuralfx::raytracing {
inline constexpr std::uint32_t max_gpu_bounces = 16;

struct alignas(16) GpuTraceConstants {
    Float3 camera_position;
    std::uint32_t width;
    Float3 camera_forward;
    std::uint32_t height;
    Float3 camera_horizontal;
    std::uint32_t node_count;
    Float3 camera_vertical;
    std::uint32_t light_count;
    Float3 ambient;
    float epsilon;
    Float3 background_bottom;
    std::uint32_t max_bounces;
    Float3 background_top;
    std::uint32_t material_count;
};

static_assert(sizeof(GpuTraceConstants) == 112);

std::optional<GpuTraceConstants> make_trace_constants(const Camera& camera, int width, int height,
    const Settings& settings, const GpuSceneCounts& counts) noexcept;
}
