#pragma once

#include "temporal/staging_ring.hpp"

#include <d3d11.h>
#include <cstdint>
#include <vector>
#include <wrl/client.h>

namespace neuralfx::temporal {
class ResolveCapture {
public:
    ResolveCapture() noexcept;
    bool capture(ID3D11DeviceContext* context, std::uint64_t frame) noexcept;
    Microsoft::WRL::ComPtr<ID3D11Buffer> resolve() const noexcept { return resolve_; }
    std::vector<FrameFloats> collect(ID3D11DeviceContext* context) noexcept { return ring_.collect(context); }

private:
    bool prepare(ID3D11DeviceContext* context) noexcept;

    Microsoft::WRL::ComPtr<ID3D11Buffer> resolve_;
    StagingRing ring_;
};
}
