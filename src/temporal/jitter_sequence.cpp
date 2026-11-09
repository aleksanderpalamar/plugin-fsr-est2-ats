#include "temporal/jitter_sequence.hpp"

#include <cmath>

namespace neuralfx::temporal {
namespace {
constexpr double plastic = 1.32471795724474602596;
constexpr double step_x = 1.0 / plastic;
constexpr double step_y = 1.0 / (plastic * plastic);
constexpr float tolerance = 2e-3f;

float centered(double step, std::uint32_t index) {
    double value = 0.5 + step * static_cast<double>(index);
    return static_cast<float>(value - std::floor(value) - 0.5);
}
}

Jitter sequence_jitter(std::uint32_t phase) noexcept {
    std::uint32_t index = phase % jitter_period + 1;
    return {centered(step_x, index), -centered(step_y, index)};
}

std::optional<std::uint32_t> sequence_phase(Jitter observed) noexcept {
    for (std::uint32_t phase = 0; phase < jitter_period; ++phase) {
        auto expected = sequence_jitter(phase);
        if (std::fabs(expected.x - observed.x) <= tolerance && std::fabs(expected.y - observed.y) <= tolerance)
            return phase;
    }
    return std::nullopt;
}
}
