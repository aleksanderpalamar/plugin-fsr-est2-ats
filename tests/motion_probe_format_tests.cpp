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
    decision.jitter = temporal::Jitter{-0.4707f, -0.419f};
    decision.sync = temporal::SyncReport{10, temporal::SyncResult::Confirmed};
    return expect(diagnostics::motion_line(12, decision, temporal::RenderSize{1440, 1080}, 35),
        "Motion frame 12 status=ready render=1440x1080 jitter=-0.4707,-0.419 sync=confirmed@10 cpu_us=35", 10);
}

int missing_case() {
    temporal::TemporalDecision decision;
    decision.status = temporal::TemporalStatus::NoScene;
    if (int code = expect(diagnostics::motion_line(0, decision, std::nullopt, 7),
        "Motion frame 0 status=no-scene cpu_us=7", 20)) return code;
    decision.status = temporal::TemporalStatus::Unsynced;
    decision.sync = temporal::SyncReport{1, temporal::SyncResult::Lost};
    return expect(diagnostics::motion_line(3, decision, temporal::RenderSize{1440, 1080}, 0),
        "Motion frame 3 status=unsynced render=1440x1080 sync=lost@1 cpu_us=0", 30);
}

int names_case() {
    using temporal::TemporalStatus;
    for (auto [status, name] : {std::pair{TemporalStatus::FirstFrame, "first-frame"},
             std::pair{TemporalStatus::Resized, "resized"}, std::pair{TemporalStatus::NoCamera, "no-camera"},
             std::pair{TemporalStatus::NoJitter, "no-jitter"}, std::pair{TemporalStatus::CameraCut, "camera-cut"}}) {
        temporal::TemporalDecision decision;
        decision.status = status;
        decision.sync = temporal::SyncReport{4, temporal::SyncResult::Resynced};
        auto line = diagnostics::motion_line(1, decision, std::nullopt, 0);
        if (line != std::string("Motion frame 1 status=") + name + " sync=resynced@4 cpu_us=0") return 40;
    }
    return 0;
}

int rejection_case() {
    temporal::Matrix4 projection{};
    projection[2] = 0.00065f;
    projection[6] = -0.00078f;
    projection[15] = 1.0f;
    temporal::RejectedCamera rejected{7, {temporal::CameraFault::NotPerspective, projection}};
    if (int code = expect(diagnostics::rejection_line(rejected),
        "Motion camera rejected frame=7 reason=not-perspective row3=0,0,0,1 offsets=0.00065,-0.00078", 50))
        return code;
    rejected.rejection = {temporal::CameraFault::SingularView, {}};
    if (int code = expect(diagnostics::rejection_line(rejected),
        "Motion camera rejected frame=7 reason=singular-view", 51)) return code;
    rejected.rejection.fault = temporal::CameraFault::JitterOutOfRange;
    if (diagnostics::rejection_line(rejected).find("reason=jitter-out-of-range") == std::string::npos) return 52;
    rejected.rejection.fault = temporal::CameraFault::EmptySize;
    return diagnostics::rejection_line(rejected).find("reason=empty-size") == std::string::npos ? 53 : 0;
}

int jump_case() {
    temporal::CameraStep step{12, 12.345f, 0.5f, {1.0f, 2.0f, -3.5f}, {13.25f, 2.0f, -3.5f}};
    return expect(diagnostics::jump_line(step),
        "Motion camera jump frame=12 move_m=12.35 turn_deg=0.50 from=1.00,2.00,-3.50 to=13.25,2.00,-3.50", 60);
}

int unmatched_case() {
    temporal::UnmatchedCamera camera{21, {0.125f, -0.25f}, {}};
    camera.matrices.view[0] = 1.0f;
    camera.matrices.view_projection[15] = 2.5f;
    std::string zeros = "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";
    return expect(diagnostics::unmatched_line(camera), "Motion camera unmatched frame=21 jitter=0.125,-0.25 view=1," +
        zeros + " view_projection=" + zeros + ",2.5", 70);
}
}

int resolve_case() {
    temporal::ResolveSample sample{12, temporal::ResolveFrame{{0.0585f, 0.1619f}, 0.31f, 2.0f}, 11u};
    if (int code = expect(diagnostics::resolve_line(sample),
        "Motion resolve frame=12 jitter=0.0585,0.1619 phase=11 move_m=0.31 turn_deg=2.00", 80)) return code;
    sample.phase.reset();
    if (int code = expect(diagnostics::resolve_line(sample),
        "Motion resolve frame=12 jitter=0.0585,0.1619 phase=none move_m=0.31 turn_deg=2.00", 81)) return code;
    int code = expect(diagnostics::resolve_rejection_line({7, temporal::ResolveFault::SizeMismatch}),
        "Motion resolve rejected frame=7 reason=size-mismatch", 82);
    if (code) return code;
    code = expect(diagnostics::resolve_rejection_line({8, temporal::ResolveFault::TooShort}),
        "Motion resolve rejected frame=8 reason=too-short", 83);
    if (code) return code;
    return expect(diagnostics::resolve_rejection_line({9, temporal::ResolveFault::NotFinite}),
        "Motion resolve rejected frame=9 reason=not-finite", 84);
}

int main() {
    for (auto test_case : {ready_case, missing_case, names_case, rejection_case, jump_case, unmatched_case, resolve_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
