#pragma once

#include "temporal/matrix4.hpp"

#include <cstdint>
#include <optional>
#include <variant>

namespace neuralfx::temporal {
struct RenderSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

struct Jitter {
    float x = 0.0f;
    float y = 0.0f;
};

enum class CameraFault { EmptySize, SingularView, NotPerspective, JitterOutOfRange };

std::variant<Jitter, CameraFault> evaluate_jitter(const Matrix4& projection, RenderSize size) noexcept;
std::optional<Jitter> projection_jitter(const Matrix4& projection, RenderSize size) noexcept;
}
