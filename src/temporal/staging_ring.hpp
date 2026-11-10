#pragma once

#include "temporal/frame_floats.hpp"

#include <d3d11.h>
#include <array>
#include <optional>
#include <vector>
#include <wrl/client.h>

namespace neuralfx::temporal {
class StagingRing {
public:
    explicit StagingRing(UINT bytes) noexcept : bytes_(bytes) {}
    bool copy(ID3D11DeviceContext* context, std::uint64_t frame, ID3D11Buffer* source, UINT offset_bytes) noexcept;
    std::vector<FrameFloats> collect(ID3D11DeviceContext* context) noexcept;

private:
    struct Pending {
        Microsoft::WRL::ComPtr<ID3D11Buffer> staging;
        std::optional<std::uint64_t> frame;
    };

    bool prepare(ID3D11DeviceContext* context, Pending& slot) noexcept;
    Pending* oldest() noexcept;

    UINT bytes_;
    std::array<Pending, 4> pending_;
};
}
