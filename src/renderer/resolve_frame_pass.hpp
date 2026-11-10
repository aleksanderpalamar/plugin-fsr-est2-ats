#pragma once

#include "renderer/camera_frame_slot.hpp"
#include "temporal/camera_continuity.hpp"
#include "temporal/projection_jitter.hpp"

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace neuralfx {
class ResolveFramePass {
public:
    HRESULT initialize(ID3D11Device* device, const std::filesystem::path& shader_directory);
    ID3D11ShaderResourceView* run(ID3D11DeviceContext* context, ID3D11Buffer* game_resolve,
        temporal::RenderSize size, temporal::JumpLimits limits = {}) noexcept;
    void reset() noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> parameters_;
    CameraFrameSlot slot_;
};
}
