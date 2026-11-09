#include "temporal/temporal_history.hpp"

#include <vector>

using namespace neuralfx::temporal;

namespace {
constexpr RenderSize scene{1440, 1080};

constexpr Matrix4 view{-0.47095215f, 0.0f, -0.8821588f, -46.862404f,
    0.0f, 1.0f, 0.0f, 10.617724f,
    0.8821588f, 0.0f, -0.47095215f, -48.97045f,
    0.0f, 0.0f, 0.0f, 1.0f};

constexpr Matrix4 view_projection{-0.5872703f, 0.0f, -1.1014271f, -58.526123f,
    -0.0006845583f, 2.2190397f, 0.0003654605f, 23.599154f,
    0.00011026186f, 0.0f, -5.8864753e-05f, 0.19390413f,
    -0.8821588f, 0.0f, 0.47095215f, 48.97045f};

FrameInput valid(RenderSize size = scene) {
    return {size, CameraMatrices{view, view_projection}, true};
}

int expect(TemporalHistory& history, const FrameInput& input, TemporalStatus status, int code) {
    auto decision = history.advance(input);
    if (decision.status != status) return code;
    bool has_motion = decision.motion.has_value();
    return has_motion == (status == TemporalStatus::Ready) ? 0 : code + 1;
}

int sequence_case() {
    TemporalHistory history;
    struct Step {
        FrameInput input;
        TemporalStatus status;
    };
    std::vector<Step> steps{
        {valid(), TemporalStatus::FirstFrame},
        {valid(), TemporalStatus::Ready},
        {{scene, CameraMatrices{view, view_projection}, false}, TemporalStatus::NoJitter},
        {valid(), TemporalStatus::FirstFrame},
        {{scene, std::nullopt, true}, TemporalStatus::NoCamera},
        {valid(), TemporalStatus::FirstFrame},
        {valid(RenderSize{1280, 720}), TemporalStatus::Resized},
        {valid(RenderSize{1280, 720}), TemporalStatus::Ready},
        {{std::nullopt, std::nullopt, false}, TemporalStatus::NoScene},
        {valid(), TemporalStatus::FirstFrame},
    };
    for (size_t index = 0; index < steps.size(); ++index)
        if (int code = expect(history, steps[index].input, steps[index].status, 10 + static_cast<int>(index) * 2))
            return code;
    return 0;
}

int invalid_camera_case() {
    TemporalHistory history;
    if (int code = expect(history, valid(), TemporalStatus::FirstFrame, 40)) return code;
    Matrix4 broken = view_projection;
    broken[14] = 0.0f;
    broken[15] = 1.0f;
    if (int code = expect(history, {scene, CameraMatrices{view, broken}, true}, TemporalStatus::NoCamera, 42)) return code;
    auto decision = history.advance(valid());
    if (decision.status != TemporalStatus::FirstFrame || !decision.camera) return 44;
    return decision.camera->jitter.x < -0.46f && decision.camera->jitter.x > -0.48f ? 0 : 45;
}
}

int main() {
    for (auto test_case : {sequence_case, invalid_camera_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
