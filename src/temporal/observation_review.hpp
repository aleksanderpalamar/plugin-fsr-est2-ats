#pragma once

#include "temporal/camera_continuity.hpp"
#include "temporal/jitter_tracker.hpp"

#include <optional>
#include <span>
#include <vector>

namespace neuralfx::temporal {
struct RejectedCamera {
    std::uint64_t frame = 0;
    CameraRejection rejection;
};

struct UnmatchedCamera {
    std::uint64_t frame = 0;
    Jitter jitter;
    CameraMatrices matrices;
};

struct ObservationReport {
    std::vector<JitterObservation> jitters;
    std::vector<RejectedCamera> rejections;
    std::vector<UnmatchedCamera> unmatched;
    std::vector<CameraStep> steps;
    std::optional<float> farthest_m;
    bool jump = false;
};

class ObservationReview {
public:
    ObservationReport review(std::span<const CameraObservation> observations, RenderSize size) noexcept;
    void reset() noexcept { continuity_.reset(); }

private:
    void accept(const CameraObservation& observation, Jitter jitter, ObservationReport& report) noexcept;
    void place(const CameraObservation& observation, ObservationReport& report) noexcept;

    CameraContinuity continuity_;
};
}
