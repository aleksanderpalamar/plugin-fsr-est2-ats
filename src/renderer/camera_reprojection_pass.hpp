#pragma once

#include "renderer/camera_frame.hpp"
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
    struct Slot {
        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> output;
    };

    HRESULT create_slot(ID3D11Device* device, Slot& slot);

    Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> parameters_;
    std::array<Slot, 2> slots_;
    std::size_t current_ = 0;
};
}
