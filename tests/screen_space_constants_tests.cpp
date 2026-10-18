#include "renderer/screen_space_constants.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace neuralfx;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float actual, float expected) {
    return std::abs(actual - expected) < 1e-5f;
}

void projection_cases() {
    RaytracingSettings settings;
    settings.vertical_fov = 60.0f;
    settings.near_plane = 0.2f;
    settings.far_plane = 3000.0f;
    auto constants = make_screen_space_constants(settings, 1920, 1080, ColorEncoding::Linear);
    require(constants.has_value(), "constants built for a valid size");
    require(near(constants->width, 1920.0f) && near(constants->height, 1080.0f), "size copied");
    require(near(constants->inverse_width, 1.0f / 1920.0f), "inverse width");
    require(near(constants->inverse_height, 1.0f / 1080.0f), "inverse height");
    require(near(constants->tan_half_fov, 0.577350f), "tangent of half the vertical field of view");
    require(near(constants->aspect, 1920.0f / 1080.0f), "aspect ratio");
    require(near(constants->near_plane, 0.2f) && near(constants->far_plane, 3000.0f), "clip planes");
}

void effect_cases() {
    RaytracingSettings settings;
    settings.reflection_strength = 0.7f;
    settings.occlusion_strength = 0.4f;
    settings.bounce_strength = 0.1f;
    settings.occlusion_radius = 2.0f;
    settings.thickness = 0.3f;
    settings.max_distance = 120.0f;
    auto constants = make_screen_space_constants(settings, 64, 32, ColorEncoding::Linear);
    require(constants.has_value(), "constants built");
    require(near(constants->reflection_strength, 0.7f), "reflection strength");
    require(near(constants->occlusion_strength, 0.4f), "occlusion strength");
    require(near(constants->bounce_strength, 0.1f), "bounce strength");
    require(near(constants->occlusion_radius, 2.0f), "occlusion radius");
    require(near(constants->thickness, 0.3f), "thickness");
    require(near(constants->max_distance, 120.0f), "max distance");
}

void state_cases() {
    RaytracingSettings settings;
    auto linear = make_screen_space_constants(settings, 8, 8, ColorEncoding::Linear);
    auto encoded = make_screen_space_constants(settings, 8, 8, ColorEncoding::Srgb);
    require(linear && linear->decode_input == 0, "linear input is not decoded");
    require(encoded && encoded->decode_input == 1, "sRGB input is decoded");
    require(linear->depth_convention == 0 && linear->debug_view == 0, "default states");
    settings.depth = DepthConvention::Reversed;
    settings.debug_view = TraceDebugView::Reflection;
    auto changed = make_screen_space_constants(settings, 8, 8, ColorEncoding::Linear);
    require(changed && changed->depth_convention == 1, "reversed depth flag");
    require(changed->debug_view == 4, "reflection debug view index");
    settings.debug_view = TraceDebugView::Normals;
    require(make_screen_space_constants(settings, 8, 8, ColorEncoding::Linear)->debug_view == 2,
        "normals debug view index");
}

void invalid_cases() {
    RaytracingSettings settings;
    require(!make_screen_space_constants(settings, 0, 8, ColorEncoding::Linear), "zero width rejected");
    require(!make_screen_space_constants(settings, 8, 0, ColorEncoding::Linear), "zero height rejected");
}
}

int main() {
    try {
        projection_cases();
        effect_cases();
        state_cases();
        invalid_cases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
