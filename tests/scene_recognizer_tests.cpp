#include "temporal/scene_recognizer.hpp"

#include <array>
#include <vector>

using namespace neuralfx::temporal;

namespace {
constexpr Surface backbuffer{0x100, 1920, 1080, 87};
constexpr Surface shadow{0x200, 2048, 2048, 20};
constexpr Surface probe_color{0x210, 1024, 1024, 10};
constexpr Surface probe_depth{0x220, 1024, 1024, 20};
constexpr Surface depth{0x300, 1440, 1080, 20};
constexpr std::array gbuffer{Surface{0x301, 1440, 1080, 10}, Surface{0x302, 1440, 1080, 10},
    Surface{0x303, 1440, 1080, 10}, Surface{0x304, 1440, 1080, 12}};
constexpr Surface ambient{0x310, 1440, 1080, 54};
constexpr std::array lighting{Surface{0x320, 1440, 1080, 10}, Surface{0x321, 1440, 1080, 10}};
constexpr Surface forward{0x330, 1440, 1080, 10};
constexpr std::array resolve{Surface{0x320, 1440, 1080, 10}, Surface{0x340, 1440, 1080, 10}};
constexpr Surface tonemapped{0x350, 1440, 1080, 29};

struct Step {
    std::vector<Surface> colors;
    std::optional<Surface> depth;
    PassRole role;
    std::vector<DrawAction> actions;
};

std::vector<Step> frame_steps(bool temporal) {
    std::vector<Step> steps{
        {{}, shadow, PassRole::Other, {DrawAction::None}},
        {{probe_color}, probe_depth, PassRole::Other, {DrawAction::None}},
        {{gbuffer.begin(), gbuffer.end()}, depth, PassRole::SceneGeometry, {DrawAction::None, DrawAction::None}},
        {{ambient}, depth, PassRole::Other, {DrawAction::None}},
        {{lighting.begin(), lighting.end()}, depth, PassRole::SceneLighting,
            {DrawAction::ReadCamera, DrawAction::None}},
        {{forward}, depth, PassRole::Other, {DrawAction::None}},
    };
    if (temporal) steps.push_back({{resolve.begin(), resolve.end()}, std::nullopt, PassRole::TemporalResolve, {DrawAction::None}});
    steps.push_back({{tonemapped}, std::nullopt, PassRole::Other, {DrawAction::None}});
    steps.push_back({{backbuffer}, std::nullopt, PassRole::Output, {DrawAction::None, DrawAction::None}});
    return steps;
}

int run_frame(SceneRecognizer& recognizer, bool temporal, int code) {
    recognizer.begin_frame(backbuffer);
    for (const auto& step : frame_steps(temporal)) {
        if (recognizer.bind(step.colors, step.depth) != step.role) return code;
        for (auto action : step.actions)
            if (recognizer.draw() != action) return code + 1;
    }
    auto frame = recognizer.frame();
    if (!frame.render || frame.render->width != 1440 || frame.render->height != 1080) return code + 2;
    if (frame.depth != depth.id || !frame.camera_requested) return code + 3;
    return frame.temporal_resolve == temporal ? 0 : code + 4;
}

int temporal_frame_case() {
    SceneRecognizer recognizer;
    return run_frame(recognizer, true, 10);
}

int plain_frame_case() {
    SceneRecognizer recognizer;
    if (int code = run_frame(recognizer, true, 20)) return code;
    return run_frame(recognizer, false, 30);
}

int unknown_frame_case() {
    SceneRecognizer recognizer;
    recognizer.begin_frame(backbuffer);
    std::array two{lighting[0], lighting[1]};
    if (recognizer.bind(two, depth) != PassRole::Other) return 40;
    if (recognizer.draw() != DrawAction::None) return 41;
    std::array mismatched{gbuffer[0], gbuffer[1], gbuffer[2], Surface{0x305, 720, 540, 10}};
    if (recognizer.bind(mismatched, depth) != PassRole::Other) return 42;
    auto frame = recognizer.frame();
    return !frame.render && !frame.depth && !frame.camera_requested && !frame.temporal_resolve ? 0 : 43;
}
}

int main() {
    for (auto test_case : {temporal_frame_case, plain_frame_case, unknown_frame_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
