#pragma once

#include "diagnostics/frame_trace.hpp"

#include <d3d11.h>
#include <cstdint>
#include <vector>

namespace neuralfx::diagnostics {
std::vector<ConstantSnapshot> read_constants(ID3D11DeviceContext* context, ShaderStage stage, std::uint32_t slots,
    std::uint32_t floats) noexcept;
}
