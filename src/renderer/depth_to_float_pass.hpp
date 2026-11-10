#pragma once

#include "temporal/projection_jitter.hpp"

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace neuralfx {
struct DepthToFloatTargets {
    ID3D11ShaderResourceView* depth = nullptr;
    ID3D11RenderTargetView* output = nullptr;
};

class DepthToFloatPass {
public:
    HRESULT initialize(ID3D11Device* device, const std::filesystem::path& shader_directory);
    void render(ID3D11DeviceContext* context, const DepthToFloatTargets& targets, temporal::RenderSize size) noexcept;
    void reset() noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_;
};
}
