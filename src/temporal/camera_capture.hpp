#pragma once

#include "temporal/camera_state.hpp"

#include <d3d11.h>
#include <array>
#include <cstdint>
#include <optional>
#include <wrl/client.h>

namespace neuralfx::temporal {
class CameraCapture {
public:
    bool capture(ID3D11DeviceContext* context, std::uint64_t frame) noexcept;
    Microsoft::WRL::ComPtr<ID3D11Buffer> camera() const noexcept { return camera_; }
    std::optional<CameraObservation> collect(ID3D11DeviceContext* context) noexcept;

private:
    struct Pending {
        Microsoft::WRL::ComPtr<ID3D11Buffer> staging;
        std::optional<std::uint64_t> frame;
    };

    bool prepare(ID3D11DeviceContext* context, Pending& slot) noexcept;
    Pending* oldest() noexcept;

    Microsoft::WRL::ComPtr<ID3D11Buffer> camera_;
    std::array<Pending, 4> pending_;
};
}
