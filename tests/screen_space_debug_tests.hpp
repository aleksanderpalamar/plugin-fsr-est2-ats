#pragma once

#include "raytracing_review_fixture.hpp"

namespace neuralfx::testing {
inline void debug_display_cases(Fixture& fixture, DXGI_FORMAT format, const char* entry) {
    auto scene = make_scene(width, height, Projection{});
    auto settings = settings_for(Projection{});
    settings.reflection_strength = 1.0f;
    settings.debug_view = TraceDebugView::Reflection;
    auto constants = constants_for(settings);
    auto probe = review_shader(fixture, L"raytracing_review.hlsl", "ReflectionLinearMain");
    auto reference = review_output(fixture, scene, constants, probe.Get());
    auto processed = review_output(fixture, scene, constants);
    auto expected = display_review(fixture, reference, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "CopyMain");
    auto actual = display_review(fixture, processed, format, entry);
    size_t sample = scene.index(80, 80);
    require(expected[sample][0] > 20, "reference contains a visible reflection");
    for (size_t channel = 0; channel < 4; ++channel)
        require_near(actual[sample][channel], expected[sample][channel], 1.0f,
            "debug reflection is encoded once");
}

inline void debug_srgb_cases(Fixture& fixture) {
    debug_display_cases(fixture, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "CopyMain");
}

inline void debug_unorm_cases(Fixture& fixture) {
    debug_display_cases(fixture, DXGI_FORMAT_R8G8B8A8_UNORM, "SrgbCopyMain");
}
}
