#pragma once

#include "temporal/temporal_history.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace neuralfx::diagnostics {
struct MotionTally {
    std::uint64_t frames = 0;
    std::uint64_t ready = 0;
    std::uint64_t confirmed = 0;
    std::uint64_t resynced = 0;
    std::uint64_t lost = 0;
    std::int64_t max_cpu_us = 0;
    std::optional<float> max_camera_m;

    void add(const temporal::TemporalDecision& decision, std::int64_t cpu_us,
        std::optional<float> camera_m) noexcept;
};

std::string tally_line(const MotionTally& tally);
}
