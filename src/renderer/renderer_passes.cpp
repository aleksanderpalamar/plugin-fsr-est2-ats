#include "renderer/renderer.hpp"
#include "renderer/photoreal_constants.hpp"
#include "common/logger.hpp"

#include <d3dcompiler.h>

namespace neuralfx {
namespace {
HRESULT compile(const std::filesystem::path& path, const char* entry,
    const char* profile, ID3DBlob** blob) {
    Microsoft::WRL::ComPtr<ID3DBlob> messages;
    HRESULT result = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry, profile, D3DCOMPILE_ENABLE_STRICTNESS, 0, blob, &messages);
    if (messages && messages->GetBufferSize() > 0)
        log(std::string_view(static_cast<const char*>(messages->GetBufferPointer()), messages->GetBufferSize()));
    return result;
}

HRESULT create_pixel(ID3D11Device* device, const std::filesystem::path& path,
    const char* entry, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixel) {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    HRESULT result = compile(path, entry, "ps_5_0", &blob);
    if (FAILED(result)) return result;
    return device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pixel);
}
}

HRESULT Renderer::compile_shaders() {
    auto directory = module_directory() / L"NeuralFX" / L"shaders";
    auto fullscreen = directory / L"fullscreen.hlsl";
    Microsoft::WRL::ComPtr<ID3DBlob> vertex_blob;
    HRESULT result = compile(fullscreen, "VSMain", "vs_5_0", &vertex_blob);
    if (FAILED(result)) return result;
    result = device_->CreateVertexShader(vertex_blob->GetBufferPointer(),
        vertex_blob->GetBufferSize(), nullptr, &vertex_);
    if (FAILED(result)) return result;
    result = create_pixel(device_.Get(), fullscreen, "CopyMain", copy_pixel_);
    if (FAILED(result)) return result;
    result = create_pixel(device_.Get(), fullscreen, "RcasMain", rcas_pixel_);
    if (FAILED(result)) return result;
    if (config_.mode != TestMode::Photoreal && config_.mode != TestMode::Raytracing) return S_OK;
    auto photoreal = directory / L"photoreal.hlsl";
    result = create_pixel(device_.Get(), photoreal, "PhotorealMain", photoreal_pixel_);
    if (FAILED(result)) return result;
    result = create_pixel(device_.Get(), photoreal, "FinishMain", finish_pixel_);
    if (FAILED(result) || config_.mode != TestMode::Raytracing) return result;
    return create_pixel(device_.Get(), directory / L"raytracing.hlsl", "TraceMain", trace_pixel_);
}

void Renderer::draw(ID3D11PixelShader* pixel, ID3D11ShaderResourceView* input,
    ID3D11RenderTargetView* output, ID3D11ShaderResourceView* neural,
    ID3D11ShaderResourceView* lut, ID3D11Buffer* parameters) {
    D3D11_VIEWPORT viewport{0, 0, static_cast<float>(resources_.width()),
        static_cast<float>(resources_.height()), 0, 1};
    D3D11_RECT scissor{0, 0, static_cast<LONG>(resources_.width()),
        static_cast<LONG>(resources_.height())};
    ID3D11ShaderResourceView* inputs[3]{input, neural, lut};
    ID3D11Buffer* constants = parameters ? parameters : constants_.Get();
    ID3D11SamplerState* sampler = sampler_.Get();
    context_->OMSetRenderTargets(1, &output, nullptr);
    context_->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    context_->OMSetDepthStencilState(nullptr, 0);
    context_->RSSetState(nullptr);
    context_->RSSetViewports(1, &viewport);
    context_->RSSetScissorRects(1, &scissor);
    context_->IASetInputLayout(nullptr);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vertex_.Get(), nullptr, 0);
    context_->HSSetShader(nullptr, nullptr, 0);
    context_->DSSetShader(nullptr, nullptr, 0);
    context_->GSSetShader(nullptr, nullptr, 0);
    context_->PSSetShader(pixel, nullptr, 0);
    context_->PSSetShaderResources(0, 3, inputs);
    context_->PSSetSamplers(0, 1, &sampler);
    context_->PSSetConstantBuffers(0, 1, &constants);
    context_->Draw(3, 0);
    ID3D11ShaderResourceView* empty[3]{};
    context_->PSSetShaderResources(0, 3, empty);
}

void Renderer::render_photoreal(ID3D11ShaderResourceView* input) {
    auto parameters = make_photoreal_constants(config_, resources_.needs_srgb_conversion(),
        !resources_.needs_srgb_conversion(), neural_residual_ != nullptr,
        lut_.size(), frame_index_);
    context_->UpdateSubresource(constants_.Get(), 0, nullptr, &parameters, 0, 0);
    draw(photoreal_pixel_.Get(), input, resources_.stage_output(),
        neural_residual_.Get(), lut_.view());
    draw(finish_pixel_.Get(), resources_.stage_input(), resources_.output());
    frame_index_ = frame_index_ >= 4095.0f ? 0.0f : frame_index_ + 1.0f;
}
}
