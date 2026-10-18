#include "config/config.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

namespace neuralfx {
namespace {
std::string_view trim(std::string_view value) noexcept {
    auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::optional<float> parse_number(std::string_view value) noexcept {
    float parsed = 0;
    auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return std::nullopt;
    if (!std::isfinite(parsed)) return std::nullopt;
    return parsed;
}

template <typename Settings>
struct Parameter {
    std::string_view name;
    float Settings::* member;
    float minimum;
    float maximum;
};

template <typename Value>
struct Choice {
    std::string_view name;
    Value value;
};

constexpr std::array photoreal_parameters{
    Parameter<PhotorealSettings>{"exposure_ev", &PhotorealSettings::exposure_ev, -2.0f, 2.0f},
    Parameter<PhotorealSettings>{"contrast", &PhotorealSettings::contrast, 0.0f, 2.0f},
    Parameter<PhotorealSettings>{"saturation", &PhotorealSettings::saturation, 0.0f, 2.0f},
    Parameter<PhotorealSettings>{"clarity", &PhotorealSettings::clarity, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"highlight_boost", &PhotorealSettings::highlight_boost, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"highlight_warmth", &PhotorealSettings::highlight_warmth, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"shadow_coolness", &PhotorealSettings::shadow_coolness, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"neural_strength", &PhotorealSettings::neural_strength, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"black_level", &PhotorealSettings::black_level, 0.0f, 0.01f},
    Parameter<PhotorealSettings>{"lut_strength", &PhotorealSettings::lut_strength, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"grain_strength", &PhotorealSettings::grain_strength, 0.0f, 0.02f},
    Parameter<PhotorealSettings>{"photoreal_sharpness", &PhotorealSettings::photoreal_sharpness, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"tone_strength", &PhotorealSettings::tone_strength, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"highlight_start", &PhotorealSettings::highlight_start, 0.0f, 1.0f},
    Parameter<PhotorealSettings>{"highlight_end", &PhotorealSettings::highlight_end, 0.0f, 1.0f}
};

constexpr std::array raytracing_parameters{
    Parameter<RaytracingSettings>{"rt_reflection_strength", &RaytracingSettings::reflection_strength, 0.0f, 1.0f},
    Parameter<RaytracingSettings>{"rt_occlusion_strength", &RaytracingSettings::occlusion_strength, 0.0f, 1.0f},
    Parameter<RaytracingSettings>{"rt_bounce_strength", &RaytracingSettings::bounce_strength, 0.0f, 1.0f},
    Parameter<RaytracingSettings>{"rt_occlusion_radius", &RaytracingSettings::occlusion_radius, 0.1f, 10.0f},
    Parameter<RaytracingSettings>{"rt_thickness", &RaytracingSettings::thickness, 0.01f, 10.0f},
    Parameter<RaytracingSettings>{"rt_max_distance", &RaytracingSettings::max_distance, 1.0f, 10000.0f},
    Parameter<RaytracingSettings>{"rt_near_plane", &RaytracingSettings::near_plane, 0.001f, 10.0f},
    Parameter<RaytracingSettings>{"rt_far_plane", &RaytracingSettings::far_plane, 1.0f, 1000000.0f},
    Parameter<RaytracingSettings>{"rt_vertical_fov", &RaytracingSettings::vertical_fov, 10.0f, 150.0f}
};

constexpr std::array depth_conventions{
    Choice<DepthConvention>{"standard", DepthConvention::Standard},
    Choice<DepthConvention>{"reversed", DepthConvention::Reversed}
};

constexpr std::array debug_views{
    Choice<TraceDebugView>{"off", TraceDebugView::Off},
    Choice<TraceDebugView>{"depth", TraceDebugView::Depth},
    Choice<TraceDebugView>{"normals", TraceDebugView::Normals},
    Choice<TraceDebugView>{"occlusion", TraceDebugView::Occlusion},
    Choice<TraceDebugView>{"reflection", TraceDebugView::Reflection}
};

template <typename Settings, size_t Count>
bool assign_parameter(Settings& settings, const std::array<Parameter<Settings>, Count>& table,
    std::string_view key, float value) noexcept {
    for (const auto& parameter : table) {
        if (key != parameter.name) continue;
        settings.*(parameter.member) = std::clamp(value, parameter.minimum, parameter.maximum);
        return true;
    }
    return false;
}

template <typename Value, size_t Count>
void assign_choice(Value& target, const std::array<Choice<Value>, Count>& table,
    std::string_view value) noexcept {
    for (const auto& choice : table) {
        if (value != choice.name) continue;
        target = choice.value;
        return;
    }
}

void assign(Config& config, std::string_view key, std::string_view value) {
    if (key == "enabled") config.enabled = value == "true" || value == "1";
    if (key == "mode" && value == "hook") config.mode = TestMode::Hook;
    if (key == "mode" && value == "copy") config.mode = TestMode::Copy;
    if (key == "mode" && value == "fullscreen") config.mode = TestMode::Fullscreen;
    if (key == "mode" && value == "rcas") config.mode = TestMode::Rcas;
    if (key == "mode" && value == "photoreal") config.mode = TestMode::Photoreal;
    if (key == "mode" && value == "raytracing") config.mode = TestMode::Raytracing;
    if (key == "lut_path") config.lut_path = value;
    if (key == "rt_depth") assign_choice(config.raytracing.depth, depth_conventions, value);
    if (key == "rt_debug") assign_choice(config.raytracing.debug_view, debug_views, value);
    auto parsed = parse_number(value);
    if (!parsed) return;
    if (key == "sharpness") config.sharpness = std::clamp(*parsed, 0.0f, 1.0f);
    if (assign_parameter(config.photoreal, photoreal_parameters, key, *parsed)) return;
    assign_parameter(config.raytracing, raytracing_parameters, key, *parsed);
}

void sanitize(PhotorealSettings& settings) noexcept {
    settings.highlight_end = std::max(0.01f, settings.highlight_end);
    if (settings.highlight_start >= settings.highlight_end)
        settings.highlight_start = settings.highlight_end - 0.01f;
}

void sanitize(RaytracingSettings& settings) noexcept {
    settings.far_plane = std::max(settings.far_plane, settings.near_plane * 2.0f);
}
}

Config parse_config(std::istream& input) {
    Config config;
    std::string line;
    while (std::getline(input, line)) {
        auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        assign(config, trim(std::string_view(line).substr(0, separator)),
            trim(std::string_view(line).substr(separator + 1)));
    }
    sanitize(config.photoreal);
    sanitize(config.raytracing);
    return config;
}
}
