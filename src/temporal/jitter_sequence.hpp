#pragma once

#include "temporal/projection_jitter.hpp"

#include <cstdint>
#include <optional>

namespace neuralfx::temporal {
inline constexpr std::uint32_t jitter_period = 16;

Jitter sequence_jitter(std::uint32_t phase) noexcept;
std::optional<std::uint32_t> sequence_phase(Jitter observed) noexcept;
}
