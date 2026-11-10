#pragma once

#include "temporal/observation_review.hpp"
#include "temporal/resolve_review.hpp"
#include "temporal/temporal_history.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace neuralfx::diagnostics {
std::string motion_line(std::uint64_t frame, const temporal::TemporalDecision& decision,
    std::optional<temporal::RenderSize> render, std::int64_t cpu_us);
std::string rejection_line(const temporal::RejectedCamera& rejected);
std::string jump_line(const temporal::CameraStep& step);
std::string unmatched_line(const temporal::UnmatchedCamera& camera);
std::string resolve_line(const temporal::ResolveSample& sample);
std::string resolve_rejection_line(const temporal::RejectedResolve& rejected);
}
