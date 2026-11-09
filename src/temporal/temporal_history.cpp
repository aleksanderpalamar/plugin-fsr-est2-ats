#include "temporal/temporal_history.hpp"

#include <utility>

namespace neuralfx::temporal {
namespace {
bool same_size(RenderSize left, RenderSize right) {
    return left.width == right.width && left.height == right.height;
}

bool predicts(TemporalStatus status) {
    return status == TemporalStatus::Ready || status == TemporalStatus::FirstFrame ||
        status == TemporalStatus::Resized;
}
}

TemporalDecision TemporalHistory::advance(const FrameInput& input) noexcept {
    TemporalDecision decision;
    decision.sync = synchronize(input.observation);
    decision.status = classify(input);
    if (predicts(decision.status)) decision.jitter = jitter_.predict(input.frame);
    return decision;
}

void TemporalHistory::reset() noexcept {
    jitter_.reset();
    size_.reset();
}

std::optional<SyncReport> TemporalHistory::synchronize(const std::optional<JitterObservation>& observation) noexcept {
    if (!observation) return std::nullopt;
    auto result = jitter_.observe(*observation);
    if (result != SyncResult::Confirmed) size_.reset();
    return SyncReport{observation->frame, result};
}

TemporalStatus TemporalHistory::classify(const FrameInput& input) noexcept {
    if (!input.render) return invalidate(TemporalStatus::NoScene);
    if (!input.camera) return invalidate(TemporalStatus::NoCamera);
    if (!input.temporal_resolve) {
        jitter_.reset();
        return invalidate(TemporalStatus::NoJitter);
    }
    if (!jitter_.predict(input.frame)) return invalidate(TemporalStatus::Unsynced);
    auto previous = std::exchange(size_, input.render);
    if (!previous) return TemporalStatus::FirstFrame;
    return same_size(*previous, *input.render) ? TemporalStatus::Ready : TemporalStatus::Resized;
}

TemporalStatus TemporalHistory::invalidate(TemporalStatus status) noexcept {
    size_.reset();
    return status;
}
}
