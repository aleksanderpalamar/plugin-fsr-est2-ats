#include "diagnostics/motion_tally.hpp"
#include "temporal/resolve_review.hpp"
#include "temporal/stage_times.hpp"

#include <iostream>

using namespace neuralfx;
using temporal::SyncResult;
using temporal::TemporalStatus;

namespace {
temporal::TemporalDecision decision(TemporalStatus status, std::optional<SyncResult> sync) {
    temporal::TemporalDecision result;
    result.status = status;
    if (sync) result.sync = temporal::SyncReport{0, *sync};
    return result;
}

temporal::ObservationReport report(std::optional<float> farthest, std::vector<temporal::CameraStep> steps = {},
    std::size_t rejected = 0) {
    temporal::ObservationReport result;
    result.farthest_m = farthest;
    result.steps = std::move(steps);
    result.rejections.resize(rejected);
    for (const auto& step : result.steps) result.jump = result.jump || temporal::is_jump(step);
    return result;
}

int summary_case() {
    diagnostics::MotionTally tally;
    tally.add(decision(TemporalStatus::Unsynced, std::nullopt), 4, report(std::nullopt, {}, 2));
    tally.add(decision(TemporalStatus::FirstFrame, SyncResult::Resynced), 9, report(70.4f));
    tally.add(decision(TemporalStatus::Ready, SyncResult::Confirmed), 3,
        report(812.6f, {{3, 0.4f, 1.25f, {}, {}}}));
    tally.add(decision(TemporalStatus::CameraCut, SyncResult::Confirmed), 2,
        report(64.0f, {{4, 12.0f, 0.5f, {}, {}}, {5, 0.3f, 0.25f, {}, {}}}));
    tally.add(decision(TemporalStatus::Unsynced, SyncResult::Lost), 1, report(std::nullopt));
    auto line = diagnostics::tally_line(tally);
    std::string wanted = "Motion summary frames=5 ready=1 cuts=1 confirmed=2 resynced=1 lost=1 rejected=2 jumps=1 "
        "max_cpu_us=9 max_move_m=12.00 max_turn_deg=1.25 max_camera_m=813 resolves=0 resolve_matched=0 "
        "resolve_rejected=0 max_capture_us=0 max_camera_collect_us=0 max_resolve_collect_us=0";
    if (line == wanted) return 0;
    std::cerr << line << '\n';
    return 10;
}

int empty_case() {
    diagnostics::MotionTally tally;
    tally.add(decision(TemporalStatus::NoScene, std::nullopt), 0, report(std::nullopt));
    auto line = diagnostics::tally_line(tally);
    if (line == "Motion summary frames=1 ready=0 cuts=0 confirmed=0 resynced=0 lost=0 rejected=0 jumps=0 max_cpu_us=0 "
        "resolves=0 resolve_matched=0 resolve_rejected=0 max_capture_us=0 max_camera_collect_us=0 "
        "max_resolve_collect_us=0")
        return 0;
    std::cerr << line << '\n';
    return 20;
}

temporal::ResolveReport resolves(std::vector<temporal::ResolveSample> samples, std::size_t rejected = 0) {
    temporal::ResolveReport result;
    result.samples = std::move(samples);
    result.rejected.resize(rejected);
    return result;
}

int resolve_case() {
    diagnostics::MotionTally tally;
    tally.add_resolves(resolves({{1, {{}, 0.05f, 0.1f}, 3u}, {2, {{}, 0.31f, 0.25f}, std::nullopt}}, 1));
    tally.add_resolves(resolves({{3, {{}, 0.12f, 1.5f}, 4u}}));
    auto line = diagnostics::tally_line(tally);
    std::string wanted = "Motion summary frames=0 ready=0 cuts=0 confirmed=0 resynced=0 lost=0 rejected=0 jumps=0 "
        "max_cpu_us=0 resolves=3 resolve_matched=2 resolve_rejected=1 max_resolve_move_m=0.31 "
        "max_resolve_turn_deg=1.50 max_capture_us=0 max_camera_collect_us=0 max_resolve_collect_us=0";
    if (line == wanted) return 0;
    std::cerr << line << '\n';
    return 30;
}
}

int times_case() {
    diagnostics::MotionTally tally;
    tally.add_times({12, 40, 7});
    tally.add_times({30, 5, 9});
    tally.add_times({4, 55, 2});
    auto line = diagnostics::tally_line(tally);
    std::string wanted = "Motion summary frames=0 ready=0 cuts=0 confirmed=0 resynced=0 lost=0 rejected=0 jumps=0 "
        "max_cpu_us=0 resolves=0 resolve_matched=0 resolve_rejected=0 max_capture_us=30 max_camera_collect_us=55 "
        "max_resolve_collect_us=9";
    if (line == wanted) return 0;
    std::cerr << line << '\n';
    return 40;
}

int main() {
    for (auto test_case : {summary_case, empty_case, resolve_case, times_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
