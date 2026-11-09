#include "temporal/jitter_tracker.hpp"

namespace neuralfx::temporal {
SyncResult JitterTracker::observe(const JitterObservation& observation) noexcept {
    auto phase = sequence_phase(observation.jitter);
    if (!phase) {
        anchor_.reset();
        return SyncResult::Lost;
    }
    if (phase_at(observation.frame) == phase) return SyncResult::Confirmed;
    anchor_ = Anchor{observation.frame, *phase};
    return SyncResult::Resynced;
}

std::optional<Jitter> JitterTracker::predict(std::uint64_t frame) const noexcept {
    auto phase = phase_at(frame);
    if (!phase) return std::nullopt;
    return sequence_jitter(*phase);
}

std::optional<std::uint32_t> JitterTracker::phase_at(std::uint64_t frame) const noexcept {
    if (!anchor_) return std::nullopt;
    auto distance = static_cast<std::int64_t>(frame - anchor_->frame) % static_cast<std::int64_t>(jitter_period);
    auto offset = static_cast<std::int64_t>(anchor_->phase) + distance + jitter_period;
    return static_cast<std::uint32_t>(offset % jitter_period);
}
}
