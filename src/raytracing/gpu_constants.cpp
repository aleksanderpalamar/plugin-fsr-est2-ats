#include "raytracing/gpu_constants.hpp"

#include <algorithm>

namespace neuralfx::raytracing {
std::optional<GpuTraceConstants> make_trace_constants(const Camera& camera, int width, int height,
    const Settings& settings, const GpuSceneCounts& counts) noexcept {
    auto frame = camera.frame(width, height);
    if (!frame) return std::nullopt;
    auto bounces = static_cast<std::uint32_t>(std::min<size_t>(settings.max_bounces, max_gpu_bounces));
    return GpuTraceConstants{
        to_float3(frame->position), static_cast<std::uint32_t>(width),
        to_float3(frame->forward), static_cast<std::uint32_t>(height),
        to_float3(frame->horizontal), counts.nodes,
        to_float3(frame->vertical), counts.lights,
        to_float3(settings.ambient), static_cast<float>(settings.epsilon),
        to_float3(settings.background_bottom), bounces,
        to_float3(settings.background_top), counts.materials
    };
}
}
