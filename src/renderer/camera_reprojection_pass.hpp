#pragma once

#include "renderer/camera_frame_slot.hpp"
#include "temporal/projection_jitter.hpp"

#include <d3d11.h>
#include <array>
#include <filesystem>
#include <wrl/client.h>

namespace neuralfx {
class CameraReprojectionPass {
public:
    HRESULT initialize(ID3D11Device* device, const std::filesystem::path& shader_directory);
    ID3D11ShaderResourceView* run(ID3D11DeviceContext* context, ID3D11Buffer* game_camera,
        temporal::RenderSize size, bool history_valid) noexcept;
    void reset() noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> parameters_;
    std::array<CameraFrameSlot, 2> slots_;
    std::size_t current_ = 0;
};
}
