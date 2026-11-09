#pragma once

#include "temporal/temporal_history.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace neuralfx::diagnostics {
std::string motion_line(std::uint64_t frame, const temporal::TemporalDecision& decision,
    std::optional<temporal::RenderSize> render, std::int64_t wait_us);
}
