#include "temporal/temporal_history.hpp"

#include <utility>

namespace neuralfx::temporal {
namespace {
bool same_size(RenderSize left, RenderSize right) {
    return left.width == right.width && left.height == right.height;
}

bool predicts(TemporalStatus status) {
    return status == TemporalStatus::Ready || status == TemporalStatus::FirstFrame ||
        status == TemporalStatus::Resized || status == TemporalStatus::CameraCut;
}

int severity(SyncResult result) {
    switch (result) {
    case SyncResult::Confirmed: return 0;
    case SyncResult::Resynced: return 1;
    case SyncResult::Lost: return 2;
    }
    return 2;
}
}

TemporalDecision TemporalHistory::advance(const FrameInput& input) noexcept {
    TemporalDecision decision;
    decision.sync = synchronize(input.observations);
    decision.status = classify(input);
    if (predicts(decision.status)) decision.jitter = jitter_.predict(input.frame);
    return decision;
}

void TemporalHistory::reset() noexcept {
    jitter_.reset();
    size_.reset();
}

std::optional<SyncReport> TemporalHistory::synchronize(const std::vector<JitterObservation>& observations) noexcept {
    std::optional<SyncReport> report;
    for (const auto& observation : observations) {
        auto result = jitter_.observe(observation);
        if (result != SyncResult::Confirmed) size_.reset();
        if (!report || severity(result) >= severity(report->result)) report = SyncReport{observation.frame, result};
    }
    return report;
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
    if (input.camera_jump) return TemporalStatus::CameraCut;
    return same_size(*previous, *input.render) ? TemporalStatus::Ready : TemporalStatus::Resized;
}

TemporalStatus TemporalHistory::invalidate(TemporalStatus status) noexcept {
    size_.reset();
    return status;
}
}
