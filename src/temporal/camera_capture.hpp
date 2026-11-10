#pragma once

#include "temporal/camera_state.hpp"
#include "temporal/staging_ring.hpp"

#include <d3d11.h>
#include <cstdint>
#include <vector>
#include <wrl/client.h>

namespace neuralfx::temporal {
class CameraCapture {
public:
    CameraCapture() noexcept;
    bool capture(ID3D11DeviceContext* context, std::uint64_t frame) noexcept;
    Microsoft::WRL::ComPtr<ID3D11Buffer> camera() const noexcept { return camera_; }
    std::vector<CameraObservation> collect(ID3D11DeviceContext* context) noexcept;

private:
    bool prepare(ID3D11DeviceContext* context) noexcept;

    Microsoft::WRL::ComPtr<ID3D11Buffer> camera_;
    StagingRing ring_;
};
}
