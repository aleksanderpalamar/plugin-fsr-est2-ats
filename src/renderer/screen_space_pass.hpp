#pragma once

#include "renderer/screen_space_constants.hpp"

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace neuralfx {
struct ScreenSpaceTargets {
    ID3D11ShaderResourceView* color = nullptr;
    ID3D11ShaderResourceView* depth = nullptr;
    ID3D11RenderTargetView* output = nullptr;
    UINT width = 0;
    UINT height = 0;
};

class ScreenSpacePass {
public:
    HRESULT initialize(ID3D11Device* device, const std::filesystem::path& shader_directory);
    void render(ID3D11DeviceContext* context, const ScreenSpaceTargets& targets,
        const ScreenSpaceConstants& constants) noexcept;
    void reset() noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;
};
}
