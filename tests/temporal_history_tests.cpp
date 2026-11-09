#include "temporal/temporal_history.hpp"

#include <vector>

using namespace neuralfx::temporal;

namespace {
constexpr RenderSize scene{1440, 1080};

FrameInput frame(std::uint64_t index, std::optional<JitterObservation> observation = std::nullopt,
    RenderSize size = scene) {
    FrameInput input{index, size, true, true, {}, false};
    if (observation) input.observations.push_back(*observation);
    return input;
}

JitterObservation seen(std::uint64_t index, std::uint32_t phase) {
    return {index, sequence_jitter(phase)};
}

int expect(TemporalHistory& history, const FrameInput& input, TemporalStatus status, int code) {
    auto decision = history.advance(input);
    if (decision.status != status) return code;
    bool predicted = decision.jitter.has_value();
    bool wanted = status == TemporalStatus::Ready || status == TemporalStatus::FirstFrame ||
        status == TemporalStatus::Resized || status == TemporalStatus::CameraCut;
    return predicted == wanted ? 0 : code + 1;
}

int sequence_case() {
    TemporalHistory history;
    struct Step {
        FrameInput input;
        TemporalStatus status;
    };
    std::vector<Step> steps{
        {frame(0), TemporalStatus::Unsynced},
        {frame(1), TemporalStatus::Unsynced},
        {frame(2, seen(0, 7)), TemporalStatus::FirstFrame},
        {frame(3, seen(1, 8)), TemporalStatus::Ready},
        {{4, scene, true, false, {}, false}, TemporalStatus::NoJitter},
        {frame(5), TemporalStatus::Unsynced},
        {frame(6, seen(4, 0)), TemporalStatus::FirstFrame},
        {{7, scene, false, true, {}, false}, TemporalStatus::NoCamera},
        {frame(8), TemporalStatus::FirstFrame},
        {frame(9, std::nullopt, RenderSize{1280, 720}), TemporalStatus::Resized},
        {frame(10, std::nullopt, RenderSize{1280, 720}), TemporalStatus::Ready},
        {{11, std::nullopt, false, false, {}, false}, TemporalStatus::NoScene},
        {frame(12), TemporalStatus::FirstFrame},
        {frame(13), TemporalStatus::Ready},
    };
    for (size_t index = 0; index < steps.size(); ++index)
        if (int code = expect(history, steps[index].input, steps[index].status, 10 + static_cast<int>(index) * 2))
            return code;
    return 0;
}

int prediction_case() {
    TemporalHistory history;
    history.advance(frame(0));
    auto decision = history.advance(frame(3, seen(1, 14)));
    if (!decision.sync || decision.sync->frame != 1 || decision.sync->result != SyncResult::Resynced) return 50;
    auto expected = sequence_jitter(0);
    if (!decision.jitter || decision.jitter->x != expected.x || decision.jitter->y != expected.y) return 51;
    decision = history.advance(frame(4, seen(2, 15)));
    if (decision.status != TemporalStatus::Ready || decision.sync->result != SyncResult::Confirmed) return 52;
    return history.advance(frame(5)).sync ? 53 : 0;
}

int resync_case() {
    TemporalHistory history;
    history.advance(frame(0, seen(0, 3)));
    if (int code = expect(history, frame(1), TemporalStatus::Ready, 60)) return code;
    if (int code = expect(history, frame(2, seen(1, 9)), TemporalStatus::FirstFrame, 62)) return code;
    if (int code = expect(history, frame(3), TemporalStatus::Ready, 64)) return code;
    if (int code = expect(history, frame(4, JitterObservation{3, Jitter{}}), TemporalStatus::Unsynced, 66)) return code;
    return 0;
}

int cut_case() {
    TemporalHistory history;
    history.advance(frame(0, seen(0, 3)));
    history.advance(frame(1));
    auto input = frame(2);
    input.camera_jump = true;
    if (int code = expect(history, input, TemporalStatus::CameraCut, 70)) return code;
    if (int code = expect(history, frame(3), TemporalStatus::Ready, 72)) return code;
    input = frame(4);
    input.camera_jump = true;
    input.temporal_resolve = false;
    return expect(history, input, TemporalStatus::NoJitter, 74);
}

int several_observations_case() {
    TemporalHistory history;
    history.advance(frame(0, seen(0, 3)));
    auto input = frame(3);
    input.observations = {seen(1, 4), seen(2, 9), seen(3, 10)};
    auto decision = history.advance(input);
    if (decision.status != TemporalStatus::FirstFrame) return 80;
    if (!decision.sync || decision.sync->result != SyncResult::Resynced || decision.sync->frame != 2) return 81;
    auto expected = sequence_jitter(10);
    return decision.jitter && decision.jitter->x == expected.x ? 0 : 82;
}

int rejected_case() {
    TemporalHistory history;
    history.advance(frame(0, seen(0, 3)));
    if (int code = expect(history, frame(1), TemporalStatus::Ready, 90)) return code;
    auto input = frame(2);
    input.camera_rejected = true;
    if (int code = expect(history, input, TemporalStatus::Unsynced, 92)) return code;
    if (int code = expect(history, frame(3), TemporalStatus::Unsynced, 94)) return code;
    return expect(history, frame(4, seen(3, 6)), TemporalStatus::FirstFrame, 96);
}
}

int main() {
    for (auto test_case : {sequence_case, prediction_case, resync_case, cut_case, several_observations_case,
             rejected_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
