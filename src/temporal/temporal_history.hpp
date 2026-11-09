#pragma once

#include "temporal/jitter_tracker.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace neuralfx::temporal {
enum class TemporalStatus { Ready, FirstFrame, Resized, CameraCut, NoScene, NoCamera, NoJitter, Unsynced };

struct FrameInput {
    std::uint64_t frame = 0;
    std::optional<RenderSize> render;
    bool camera = false;
    bool temporal_resolve = false;
    std::vector<JitterObservation> observations;
    bool camera_jump = false;
};

struct SyncReport {
    std::uint64_t frame = 0;
    SyncResult result = SyncResult::Lost;
};

struct TemporalDecision {
    TemporalStatus status = TemporalStatus::NoScene;
    std::optional<Jitter> jitter;
    std::optional<SyncReport> sync;
};

class TemporalHistory {
public:
    TemporalDecision advance(const FrameInput& input) noexcept;
    void reset() noexcept;

private:
    std::optional<SyncReport> synchronize(const std::vector<JitterObservation>& observations) noexcept;
    TemporalStatus classify(const FrameInput& input) noexcept;
    TemporalStatus invalidate(TemporalStatus status) noexcept;

    JitterTracker jitter_;
    std::optional<RenderSize> size_;
};
}
