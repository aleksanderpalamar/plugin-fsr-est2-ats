#include "common/logger.hpp"
#include "screen_space_pass_fixture.hpp"

#include <array>
#include <iostream>

using namespace neuralfx;
using namespace neuralfx::testing;

namespace {
constexpr Band near_ground{Surface::Ground, 3.0f, 5.0f};
constexpr Band ground_before_wall{Surface::Ground, 6.0f, 10.5f};
constexpr Band wall_base{Surface::Ground, 10.8f, 11.9f};
constexpr Band wall{Surface::Wall, 0.0f, 100.0f};

void identity_cases(Fixture& fixture) {
    auto scene = make_scene(width, height, Projection{});
    auto output = run(fixture, scene, constants_for(settings_for(Projection{})));
    for (size_t index = 0; index < output.size(); ++index)
        for (size_t channel = 0; channel < 3; ++channel)
            require_near(output[index][channel], scene.color[index][channel], 1e-6f,
                "zero strengths keep the image");
}

void srgb_input_cases(Fixture& fixture) {
    auto scene = make_scene(width, height, Projection{});
    std::vector<std::array<unsigned char, 4>> encoded(scene.color.size(), {128, 128, 128, 255});
    auto color = create_input(fixture.gpu.device.Get(), width, height,
        DXGI_FORMAT_R8G8B8A8_UNORM, encoded.data(), width * 4);
    auto output = run(fixture, scene, color.Get(),
        constants_for(settings_for(Projection{}), ColorEncoding::Srgb));
    require_near(output[scene.index(80, 100)][0], 0.21586f, 0.002f, "sRGB input written as linear");
}

void background_cases(Fixture& fixture) {
    for (DepthConvention depth : {DepthConvention::Standard, DepthConvention::Reversed}) {
        Projection projection{.depth = depth};
        auto scene = make_scene(width, height, projection);
        auto settings = settings_for(projection);
        settings.reflection_strength = 1.0f;
        settings.occlusion_strength = 1.0f;
        settings.bounce_strength = 1.0f;
        auto output = run(fixture, scene, constants_for(settings));
        auto sky = scene.index(80, 2);
        require(scene.surfaces[sky] == Surface::Sky, "sky sampled");
        for (size_t channel = 0; channel < 3; ++channel)
            require_near(output[sky][channel], sky_color[channel], 1e-6f, "sky stays untouched");
    }
}

void depth_view_cases(Fixture& fixture) {
    for (DepthConvention depth : {DepthConvention::Standard, DepthConvention::Reversed}) {
        Projection projection{.depth = depth};
        auto scene = make_scene(width, height, projection);
        auto settings = settings_for(projection);
        settings.debug_view = TraceDebugView::Depth;
        auto output = run(fixture, scene, constants_for(settings));
        for (UINT y : {70u, 90u, 110u}) {
            size_t index = scene.index(80, y);
            require_near(output[index][0], scene.view_depth[index] / settings.max_distance, 1e-3f,
                "depth view shows linear distance");
        }
    }
}

void normals_view_cases(Fixture& fixture) {
    auto scene = make_scene(width, height, Projection{});
    auto settings = settings_for(Projection{});
    settings.debug_view = TraceDebugView::Normals;
    auto output = run(fixture, scene, constants_for(settings));
    const Pixel& ground = output[scene.index(80, 100)];
    require_near(ground[0], 0.5f, 0.02f, "ground normal x");
    require_near(ground[1], 1.0f, 0.02f, "ground normal points up");
    require_near(ground[2], 0.5f, 0.02f, "ground normal z");
    const Pixel& facing = output[scene.index(80, 55)];
    require_near(facing[0], 0.5f, 0.02f, "wall normal x");
    require_near(facing[1], 0.5f, 0.02f, "wall normal y");
    require_near(facing[2], 0.0f, 0.02f, "wall normal faces the camera");
}

void reflection_cases(Fixture& fixture) {
    auto scene = make_scene(width, height, Projection{});
    auto settings = settings_for(Projection{});
    settings.reflection_strength = 1.0f;
    auto output = run(fixture, scene, constants_for(settings));
    require_above(channel_gain(scene, output, ground_before_wall, 0), 0.05f,
        "ground in front of the wall reflects the wall");
}

void occlusion_cases(Fixture& fixture) {
    auto scene = make_scene(width, height, Projection{});
    auto settings = settings_for(Projection{});
    settings.occlusion_strength = 1.0f;
    auto output = run(fixture, scene, constants_for(settings));
    float open = channel_gain(scene, output, near_ground, 1);
    float corner = channel_gain(scene, output, wall_base, 1);
    require_near(open, 0.0f, 0.005f, "open ground stays unoccluded");
    require_above(open - corner, 0.01f, "corner at the wall base is darker");
    require_above(-channel_gain(scene, output, wall, 1), 0.0f, "wall above the ground is occluded");
}

void bounce_cases(Fixture& fixture) {
    auto scene = make_scene(width, height, Projection{});
    auto settings = settings_for(Projection{});
    settings.bounce_strength = 1.0f;
    auto output = run(fixture, scene, constants_for(settings));
    require_near(channel_gain(scene, output, near_ground, 0), 0.0f, 0.005f,
        "open ground receives no bounce");
    require_above(channel_gain(scene, output, wall_base, 0), 0.002f,
        "ground at the wall base receives red bounce light");
}
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) {
        std::cerr << "D3D11 device unavailable\n";
        return 1;
    }
    Fixture fixture{*gpu, {}};
    auto shaders = module_directory() / L"NeuralFX" / L"shaders";
    if (FAILED(fixture.pass.initialize(gpu->device.Get(), shaders))) {
        std::cerr << "Screen-space pass initialization failed\n";
        return 2;
    }
    constexpr std::array cases{identity_cases, srgb_input_cases, background_cases,
        depth_view_cases, normals_view_cases, reflection_cases, occlusion_cases, bounce_cases};
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
