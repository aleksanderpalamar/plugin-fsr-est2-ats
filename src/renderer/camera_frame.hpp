#pragma once

#include "temporal/matrix4.hpp"

#include <cstdint>

namespace neuralfx {
struct GpuCameraFrame {
    temporal::Matrix4 reprojection{};
    temporal::Matrix4 view{};
    temporal::Matrix4 projection{};
    float jitter[2]{};
    std::uint32_t valid = 0;
    std::uint32_t padding = 0;
};
static_assert(sizeof(GpuCameraFrame) == 208);
}
