#pragma once

#include "temporal/projection_jitter.hpp"

#include <cstddef>
#include <span>
#include <variant>

namespace neuralfx::temporal {
inline constexpr std::size_t resolve_floats = 64;

struct ResolveFrame {
    Jitter jitter;
    float move_m = 0.0f;
    float turn_deg = 0.0f;
};

enum class ResolveFault { TooShort, NotFinite, SizeMismatch };

std::variant<ResolveFrame, ResolveFault> evaluate_resolve(std::span<const float> values, RenderSize expected) noexcept;
}
