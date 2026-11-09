#pragma once

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace neuralfx {
struct MotionOverlayTargets {
    ID3D11ShaderResourceView* motion = nullptr;
    ID3D11RenderTargetView* output = nullptr;
    UINT width = 0;
    UINT height = 0;
};

class MotionOverlayPass {
public:
    HRESULT initialize(ID3D11Device* device, const std::filesystem::path& shader_directory);
    void render(ID3D11DeviceContext* context, const MotionOverlayTargets& targets, float scale) noexcept;
    void reset() noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;
};
}
