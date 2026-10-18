#pragma once

#include "raytracing_review_fixture.hpp"

#include <algorithm>

namespace neuralfx::testing {
inline void refinement_direction_cases(Fixture& fixture, const char* entry) {
    for (DepthConvention convention : {DepthConvention::Standard, DepthConvention::Reversed}) {
        Projection projection{.depth = convention};
        auto scene = make_scene(width, height, projection);
        std::fill(scene.device_depth.begin(), scene.device_depth.end(), encode_depth(7.0f, projection));
        auto shader = review_shader(fixture, L"raytracing_review.hlsl", entry);
        auto target = review_output(fixture, scene, constants_for(settings_for(projection)), shader.Get());
        auto pixels = read_pixels(fixture.gpu, target.texture.Get(), width, height);
        require(pixels.has_value(), "refined hit read back");
        require_near(pixels->front()[0], 7.0f, 0.03f,
            std::string(entry) + " converges to the intersected surface");
    }
}

inline void refinement_toward_cases(Fixture& fixture) {
    refinement_direction_cases(fixture, "RefineTowardMain");
}

inline void refinement_away_cases(Fixture& fixture) {
    refinement_direction_cases(fixture, "RefineAwayMain");
}
}
