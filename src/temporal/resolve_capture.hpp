#pragma once

#include "temporal/staging_ring.hpp"

#include <d3d11.h>
#include <cstdint>
#include <vector>

namespace neuralfx::temporal {
class ResolveCapture {
public:
    ResolveCapture() noexcept;
    bool capture(ID3D11DeviceContext* context, std::uint64_t frame) noexcept;
    std::vector<FrameFloats> collect(ID3D11DeviceContext* context) noexcept { return ring_.collect(context); }

private:
    StagingRing ring_;
};
}
