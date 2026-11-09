#include "diagnostics/motion_tally.hpp"

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

int summary_case() {
    diagnostics::MotionTally tally;
    tally.add(decision(TemporalStatus::Unsynced, std::nullopt), 4, std::nullopt);
    tally.add(decision(TemporalStatus::FirstFrame, SyncResult::Resynced), 9, 70.4f);
    tally.add(decision(TemporalStatus::Ready, SyncResult::Confirmed), 3, 812.6f);
    tally.add(decision(TemporalStatus::Ready, SyncResult::Confirmed), 2, 64.0f);
    tally.add(decision(TemporalStatus::Unsynced, SyncResult::Lost), 1, std::nullopt);
    auto line = diagnostics::tally_line(tally);
    std::string wanted =
        "Motion summary frames=5 ready=2 confirmed=2 resynced=1 lost=1 max_cpu_us=9 max_camera_m=813";
    if (line == wanted) return 0;
    std::cerr << line << '\n';
    return 10;
}

int empty_case() {
    diagnostics::MotionTally tally;
    tally.add(decision(TemporalStatus::NoScene, std::nullopt), 0, std::nullopt);
    auto line = diagnostics::tally_line(tally);
    if (line == "Motion summary frames=1 ready=0 confirmed=0 resynced=0 lost=0 max_cpu_us=0") return 0;
    std::cerr << line << '\n';
    return 20;
}
}

int main() {
    for (auto test_case : {summary_case, empty_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
