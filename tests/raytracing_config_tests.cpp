#include "config/config.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace neuralfx;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float actual, float expected) {
    return std::abs(actual - expected) < 1e-5f;
}

RaytracingSettings parse(const std::string& text) {
    std::istringstream input(text);
    return parse_config(input).raytracing;
}

void parsing_cases() {
    auto settings = parse("rt_reflection_strength=0.6\nrt_occlusion_strength=0.4\n"
        "rt_bounce_strength=0.2\nrt_occlusion_radius=2.5\nrt_thickness=0.75\n"
        "rt_max_distance=400\nrt_near_plane=0.05\nrt_far_plane=8000\nrt_vertical_fov=70\n");
    require(near(settings.reflection_strength, 0.6f), "reflection strength parsed");
    require(near(settings.occlusion_strength, 0.4f), "occlusion strength parsed");
    require(near(settings.bounce_strength, 0.2f), "bounce strength parsed");
    require(near(settings.occlusion_radius, 2.5f), "occlusion radius parsed");
    require(near(settings.thickness, 0.75f), "thickness parsed");
    require(near(settings.max_distance, 400.0f), "max distance parsed");
    require(near(settings.near_plane, 0.05f), "near plane parsed");
    require(near(settings.far_plane, 8000.0f), "far plane parsed");
    require(near(settings.vertical_fov, 70.0f), "vertical field of view parsed");
}

void clamping_cases() {
    auto settings = parse("rt_reflection_strength=3\nrt_occlusion_strength=-1\n"
        "rt_vertical_fov=500\nrt_near_plane=0\nrt_thickness=nan\n");
    require(near(settings.reflection_strength, 1.0f), "strength clamped to one");
    require(near(settings.occlusion_strength, 0.0f), "strength clamped to zero");
    require(near(settings.vertical_fov, 150.0f), "field of view clamped");
    require(settings.near_plane > 0.0f, "near plane stays positive");
    require(near(settings.thickness, RaytracingSettings{}.thickness), "invalid number ignored");
    auto inverted = parse("rt_near_plane=5\nrt_far_plane=1\n");
    require(inverted.far_plane > inverted.near_plane, "far plane kept beyond near plane");
}

void enum_cases() {
    require(parse("").depth == DepthConvention::Standard, "standard depth by default");
    require(parse("rt_depth=reversed\n").depth == DepthConvention::Reversed, "reversed depth parsed");
    require(parse("rt_depth=sideways\n").depth == DepthConvention::Standard, "unknown depth ignored");
    require(parse("").debug_view == TraceDebugView::Off, "debug view off by default");
    require(parse("rt_debug=depth\n").debug_view == TraceDebugView::Depth, "depth view parsed");
    require(parse("rt_debug=normals\n").debug_view == TraceDebugView::Normals, "normals view parsed");
    require(parse("rt_debug=occlusion\n").debug_view == TraceDebugView::Occlusion,
        "occlusion view parsed");
    require(parse("rt_debug=reflection\n").debug_view == TraceDebugView::Reflection,
        "reflection view parsed");
    require(parse("rt_debug=everything\n").debug_view == TraceDebugView::Off, "unknown view ignored");
}

void live_refresh_cases() {
    Config active;
    active.enabled = true;
    active.mode = TestMode::Raytracing;
    std::istringstream input("enabled=false\nmode=hook\nsharpness=0.2\nexposure_ev=0.5\n"
        "lut_path=other.cube\nrt_reflection_strength=0.9\nrt_debug=normals\n");
    Config reloaded = parse_config(input);
    refresh_live_settings(active, reloaded);
    require(active.enabled, "enabled state survives reload");
    require(active.mode == TestMode::Raytracing, "mode survives reload");
    require(near(active.sharpness, 0.2f), "sharpness reloaded");
    require(near(active.photoreal.exposure_ev, 0.5f), "photoreal settings reloaded");
    require(active.lut_path == "other.cube", "LUT path reloaded");
    require(near(active.raytracing.reflection_strength, 0.9f), "ray tracing settings reloaded");
    require(active.raytracing.debug_view == TraceDebugView::Normals, "debug view reloaded");
}
}

int main() {
    try {
        parsing_cases();
        clamping_cases();
        enum_cases();
        live_refresh_cases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
