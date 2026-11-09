#include "diagnostics/motion_probe_format.hpp"

#include <iostream>

using namespace neuralfx;

namespace {
int expect(const std::string& line, const std::string& wanted, int code) {
    if (line == wanted) return 0;
    std::cerr << "case " << code << " produced: " << line << '\n';
    return code;
}

int ready_case() {
    temporal::TemporalDecision decision;
    decision.status = temporal::TemporalStatus::Ready;
    decision.camera = temporal::CameraState{{-0.4707f, -0.419f}, {}, {}};
    return expect(diagnostics::motion_line(12, decision, temporal::RenderSize{1440, 1080}, 350),
        "Motion frame 12 status=ready render=1440x1080 jitter=-0.4707,-0.419 wait_us=350", 10);
}

int missing_case() {
    temporal::TemporalDecision decision;
    decision.status = temporal::TemporalStatus::NoScene;
    if (int code = expect(diagnostics::motion_line(0, decision, std::nullopt, 7),
        "Motion frame 0 status=no-scene wait_us=7", 20)) return code;
    decision.status = temporal::TemporalStatus::NoJitter;
    decision.camera = temporal::CameraState{};
    return expect(diagnostics::motion_line(3, decision, temporal::RenderSize{1440, 1080}, 0),
        "Motion frame 3 status=no-jitter render=1440x1080 jitter=0,0 wait_us=0", 30);
}

int status_names_case() {
    using temporal::TemporalStatus;
    for (auto [status, name] : {std::pair{TemporalStatus::FirstFrame, "first-frame"},
             std::pair{TemporalStatus::Resized, "resized"}, std::pair{TemporalStatus::NoCamera, "no-camera"}}) {
        temporal::TemporalDecision decision;
        decision.status = status;
        auto line = diagnostics::motion_line(1, decision, std::nullopt, 0);
        if (line != std::string("Motion frame 1 status=") + name + " wait_us=0") return 40;
    }
    return 0;
}
}

int main() {
    for (auto test_case : {ready_case, missing_case, status_names_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
