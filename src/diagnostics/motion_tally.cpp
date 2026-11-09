#include "diagnostics/motion_tally.hpp"

#include <algorithm>
#include <cmath>

namespace neuralfx::diagnostics {
void MotionTally::add(const temporal::TemporalDecision& decision, std::int64_t cpu_us,
    std::optional<float> camera_m) noexcept {
    ++frames;
    if (decision.status == temporal::TemporalStatus::Ready) ++ready;
    max_cpu_us = std::max(max_cpu_us, cpu_us);
    if (camera_m) max_camera_m = std::max(max_camera_m.value_or(0.0f), *camera_m);
    if (!decision.sync) return;
    switch (decision.sync->result) {
    case temporal::SyncResult::Confirmed: ++confirmed; break;
    case temporal::SyncResult::Resynced: ++resynced; break;
    case temporal::SyncResult::Lost: ++lost; break;
    }
}

std::string tally_line(const MotionTally& tally) {
    std::string line = "Motion summary frames=" + std::to_string(tally.frames) +
        " ready=" + std::to_string(tally.ready) + " confirmed=" + std::to_string(tally.confirmed) +
        " resynced=" + std::to_string(tally.resynced) + " lost=" + std::to_string(tally.lost) +
        " max_cpu_us=" + std::to_string(tally.max_cpu_us);
    if (!tally.max_camera_m) return line;
    return line + " max_camera_m=" + std::to_string(std::lround(*tally.max_camera_m));
}
}
