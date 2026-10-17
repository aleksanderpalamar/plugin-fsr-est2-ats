#pragma once

#include <d3d11.h>
#include <array>

namespace neuralfx {
struct FullscreenDraw {
    ID3D11VertexShader* vertex = nullptr;
    ID3D11PixelShader* pixel = nullptr;
    std::array<ID3D11ShaderResourceView*, 3> inputs{};
    ID3D11RenderTargetView* output = nullptr;
    ID3D11Buffer* constants = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    UINT width = 0;
    UINT height = 0;
};

void draw_fullscreen(ID3D11DeviceContext* context, const FullscreenDraw& draw) noexcept;
}
