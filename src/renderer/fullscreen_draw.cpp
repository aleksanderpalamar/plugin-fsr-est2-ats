#include "renderer/fullscreen_draw.hpp"

namespace neuralfx {
void draw_fullscreen(ID3D11DeviceContext* context, const FullscreenDraw& draw) noexcept {
    D3D11_VIEWPORT viewport{0, 0, static_cast<float>(draw.width),
        static_cast<float>(draw.height), 0, 1};
    D3D11_RECT scissor{0, 0, static_cast<LONG>(draw.width), static_cast<LONG>(draw.height)};
    ID3D11RenderTargetView* output = draw.output;
    ID3D11Buffer* constants = draw.constants;
    ID3D11SamplerState* sampler = draw.sampler;
    context->OMSetRenderTargets(1, &output, nullptr);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
    context->RSSetViewports(1, &viewport);
    context->RSSetScissorRects(1, &scissor);
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(draw.vertex, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(draw.pixel, nullptr, 0);
    context->PSSetShaderResources(0, static_cast<UINT>(draw.inputs.size()), draw.inputs.data());
    context->PSSetSamplers(0, 1, &sampler);
    context->PSSetConstantBuffers(0, 1, &constants);
    context->Draw(3, 0);
    std::array<ID3D11ShaderResourceView*, 3> empty{};
    context->PSSetShaderResources(0, static_cast<UINT>(empty.size()), empty.data());
}
}
