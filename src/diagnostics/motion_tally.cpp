#include "diagnostics/motion_tally.hpp"
#include "diagnostics/number_format.hpp"

#include <algorithm>
#include <cmath>

namespace neuralfx::diagnostics {
namespace {
void keep_largest(std::optional<float>& largest, float value) {
    largest = std::max(largest.value_or(value), value);
}

std::string count(const char* name, std::uint64_t value) {
    return std::string(" ") + name + "=" + std::to_string(value);
}
}

void MotionTally::add(const temporal::TemporalDecision& decision, std::int64_t cpu_us,
    const temporal::ObservationReport& report) noexcept {
    ++frames;
    if (decision.status == temporal::TemporalStatus::Ready) ++ready;
    if (decision.status == temporal::TemporalStatus::CameraCut) ++cuts;
    max_cpu_us = std::max(max_cpu_us, cpu_us);
    rejected += report.rejections.size();
    if (report.farthest_m) keep_largest(max_camera_m, *report.farthest_m);
    count_steps(report);
    count_sync(decision);
}

void MotionTally::add_resolves(const temporal::ResolveReport& report) noexcept {
    resolve_rejected += report.rejected.size();
    for (const auto& sample : report.samples) {
        ++resolves;
        if (sample.phase) ++resolve_matched;
        keep_largest(max_resolve_move_m, sample.values.move_m);
        keep_largest(max_resolve_turn_deg, sample.values.turn_deg);
    }
}

void MotionTally::count_sync(const temporal::TemporalDecision& decision) noexcept {
    if (!decision.sync) return;
    switch (decision.sync->result) {
    case temporal::SyncResult::Confirmed: ++confirmed; break;
    case temporal::SyncResult::Resynced: ++resynced; break;
    case temporal::SyncResult::Lost: ++lost; break;
    }
}

void MotionTally::count_steps(const temporal::ObservationReport& report) noexcept {
    for (const auto& step : report.steps) {
        if (temporal::is_jump(step)) ++jumps;
        keep_largest(max_move_m, step.distance_m);
        keep_largest(max_turn_deg, step.angle_deg);
    }
}

std::string tally_line(const MotionTally& tally) {
    std::string line = "Motion summary" + count("frames", tally.frames) + count("ready", tally.ready) +
        count("cuts", tally.cuts) + count("confirmed", tally.confirmed) + count("resynced", tally.resynced) +
        count("lost", tally.lost) + count("rejected", tally.rejected) + count("jumps", tally.jumps) +
        " max_cpu_us=" + std::to_string(tally.max_cpu_us);
    if (tally.max_move_m) line += " max_move_m=" + fixed(*tally.max_move_m);
    if (tally.max_turn_deg) line += " max_turn_deg=" + fixed(*tally.max_turn_deg);
    if (tally.max_camera_m) line += " max_camera_m=" + std::to_string(std::lround(*tally.max_camera_m));
    line += count("resolves", tally.resolves) + count("resolve_matched", tally.resolve_matched) +
        count("resolve_rejected", tally.resolve_rejected);
    if (tally.max_resolve_move_m) line += " max_resolve_move_m=" + fixed(*tally.max_resolve_move_m);
    if (tally.max_resolve_turn_deg) line += " max_resolve_turn_deg=" + fixed(*tally.max_resolve_turn_deg);
    return line;
}
}
