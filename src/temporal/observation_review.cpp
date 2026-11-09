#include "temporal/observation_review.hpp"

#include <algorithm>
#include <cmath>

namespace neuralfx::temporal {
namespace {
float length(Position position) {
    return std::sqrt(position.x * position.x + position.y * position.y + position.z * position.z);
}
}

ObservationReport ObservationReview::review(std::span<const CameraObservation> observations,
    RenderSize size) noexcept {
    ObservationReport report;
    for (const auto& observation : observations) {
        auto evaluated = evaluate_camera(observation.matrices, size);
        if (const auto* state = std::get_if<CameraState>(&evaluated))
            report.jitters.push_back({observation.frame, state->jitter});
        else
            report.rejections.push_back({observation.frame, std::get<CameraRejection>(evaluated)});
        place(observation, report);
    }
    return report;
}

void ObservationReview::place(const CameraObservation& observation, ObservationReport& report) noexcept {
    if (auto position = camera_position(observation.matrices.view))
        report.farthest_m = std::max(report.farthest_m.value_or(0.0f), length(*position));
    auto step = continuity_.observe(observation);
    if (!step) return;
    report.jump = report.jump || is_jump(*step);
    report.steps.push_back(*step);
}
}
