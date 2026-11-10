#include "diagnostics/probe_journal.hpp"
#include "diagnostics/motion_probe_format.hpp"
#include "common/logger.hpp"

namespace neuralfx::diagnostics {
namespace {
constexpr std::uint64_t logged_frames = 240;
constexpr std::uint64_t logged_events = 32;

void log_limited(std::uint64_t& used, const std::string& line) {
    if (used >= logged_events) return;
    ++used;
    log(line);
}
}

void ProbeJournal::reset() noexcept {
    frame_ = 0;
    budget_ = {};
    tally_ = {};
}

void ProbeJournal::record(const temporal::TemporalDecision& decision, const FrameFacts& facts,
    const temporal::ObservationReport& report, const temporal::ResolveReport& resolves) {
    if (frame_ < logged_frames) log(motion_line(facts.index, decision, facts.render, facts.cpu_us));
    ++frame_;
    record_events(report, resolves);
    tally_.add(decision, facts.cpu_us, report);
    tally_.add_resolves(resolves);
    tally_.add_times(facts.times);
    if (tally_.frames < logged_frames) return;
    log(tally_line(tally_));
    tally_ = {};
}

void ProbeJournal::record_events(const temporal::ObservationReport& report, const temporal::ResolveReport& resolves) {
    for (const auto& rejected : report.rejections) log_limited(budget_.rejections, rejection_line(rejected));
    for (const auto& unmatched : report.unmatched) log_limited(budget_.unmatched, unmatched_line(unmatched));
    for (const auto& step : report.steps)
        if (temporal::is_jump(step)) log_limited(budget_.jumps, jump_line(step));
    for (const auto& rejected : resolves.rejected) log_limited(budget_.resolve_faults, resolve_rejection_line(rejected));
    for (const auto& sample : resolves.samples)
        if (temporal::is_notable(sample)) log_limited(budget_.resolves, resolve_line(sample));
}
}
