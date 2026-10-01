#include "renderer/context_state.hpp"

namespace neuralfx {
ContextState::ContextState(ID3D11DeviceContext* context) noexcept : context_(context) {
    if (!context_) return;
    ID3D11RenderTargetView* raw_targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
    context_->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, raw_targets, depth_target_.GetAddressOf());
    for (UINT index = 0; index < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++index) targets_[index].Attach(raw_targets[index]);
    context_->OMGetBlendState(blend_.GetAddressOf(), blend_factor_, &sample_mask_);
    context_->OMGetDepthStencilState(depth_.GetAddressOf(), &stencil_reference_);
    context_->RSGetState(rasterizer_.GetAddressOf());
    context_->RSGetViewports(&viewport_count_, viewports_);
    context_->RSGetScissorRects(&scissor_count_, scissors_);
    context_->VSGetShader(vertex_.GetAddressOf(), nullptr, nullptr);
    context_->PSGetShader(pixel_.GetAddressOf(), nullptr, nullptr);
    context_->GSGetShader(geometry_.GetAddressOf(), nullptr, nullptr);
    context_->HSGetShader(hull_.GetAddressOf(), nullptr, nullptr);
    context_->DSGetShader(domain_.GetAddressOf(), nullptr, nullptr);
    context_->PSGetShaderResources(0, 1, resource_.GetAddressOf());
    context_->PSGetSamplers(0, 1, sampler_.GetAddressOf());
    context_->PSGetConstantBuffers(0, 1, constants_.GetAddressOf());
    context_->IAGetInputLayout(layout_.GetAddressOf());
    context_->IAGetPrimitiveTopology(&topology_);
}

ContextState::~ContextState() {
    if (!context_) return;
    ID3D11RenderTargetView* raw_targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
    for (UINT index = 0; index < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++index) raw_targets[index] = targets_[index].Get();
    context_->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, raw_targets, depth_target_.Get());
    context_->OMSetBlendState(blend_.Get(), blend_factor_, sample_mask_);
    context_->OMSetDepthStencilState(depth_.Get(), stencil_reference_);
    context_->RSSetState(rasterizer_.Get());
    context_->RSSetViewports(viewport_count_, viewports_);
    context_->RSSetScissorRects(scissor_count_, scissors_);
    context_->VSSetShader(vertex_.Get(), nullptr, 0);
    context_->PSSetShader(pixel_.Get(), nullptr, 0);
    context_->GSSetShader(geometry_.Get(), nullptr, 0);
    context_->HSSetShader(hull_.Get(), nullptr, 0);
    context_->DSSetShader(domain_.Get(), nullptr, 0);
    ID3D11ShaderResourceView* resource = resource_.Get();
    context_->PSSetShaderResources(0, 1, &resource);
    ID3D11SamplerState* sampler = sampler_.Get();
    context_->PSSetSamplers(0, 1, &sampler);
    ID3D11Buffer* constants = constants_.Get();
    context_->PSSetConstantBuffers(0, 1, &constants);
    context_->IASetInputLayout(layout_.Get());
    context_->IASetPrimitiveTopology(topology_);
}
}
