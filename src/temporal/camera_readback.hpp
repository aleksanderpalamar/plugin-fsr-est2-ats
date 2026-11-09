#pragma once

#include "temporal/camera_state.hpp"

#include <d3d11.h>
#include <optional>
#include <wrl/client.h>

namespace neuralfx::temporal {
class CameraReadback {
public:
    bool request(ID3D11DeviceContext* context) noexcept;
    std::optional<CameraMatrices> resolve(ID3D11DeviceContext* context) noexcept;
    void clear() noexcept { pending_ = false; }

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> staging_;
    bool pending_ = false;
};
}
