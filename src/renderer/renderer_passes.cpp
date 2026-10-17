#include "renderer/renderer.hpp"
#include "renderer/photoreal_constants.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"
#include "common/logger.hpp"

namespace neuralfx {
HRESULT Renderer::compile_shaders() {
    auto directory = module_directory() / L"NeuralFX" / L"shaders";
    auto fullscreen = directory / L"fullscreen.hlsl";
    HRESULT result = create_vertex_shader(device_.Get(), fullscreen, "VSMain", vertex_);
    if (FAILED(result)) return result;
    result = create_pixel_shader(device_.Get(), fullscreen, "CopyMain", copy_pixel_);
    if (FAILED(result)) return result;
    result = create_pixel_shader(device_.Get(), fullscreen, "RcasMain", rcas_pixel_);
    if (FAILED(result)) return result;
    if (config_.mode != TestMode::Photoreal && config_.mode != TestMode::Raytracing) return S_OK;
    auto photoreal = directory / L"photoreal.hlsl";
    result = create_pixel_shader(device_.Get(), photoreal, "PhotorealMain", photoreal_pixel_);
    if (FAILED(result)) return result;
    return create_pixel_shader(device_.Get(), photoreal, "FinishMain", finish_pixel_);
}

void Renderer::draw(ID3D11PixelShader* pixel, ID3D11ShaderResourceView* input,
    ID3D11RenderTargetView* output, ID3D11ShaderResourceView* neural,
    ID3D11ShaderResourceView* lut, ID3D11Buffer* parameters) {
    draw_fullscreen(context_.Get(), {vertex_.Get(), pixel, {input, neural, lut}, output,
        parameters ? parameters : constants_.Get(), sampler_.Get(),
        resources_.width(), resources_.height()});
}

void Renderer::render_photoreal(ID3D11ShaderResourceView* input, ColorEncoding encoding) {
    auto parameters = make_photoreal_constants(config_, encoding == ColorEncoding::Srgb,
        !resources_.needs_srgb_conversion(), neural_residual_ != nullptr,
        lut_.size(), frame_index_);
    context_->UpdateSubresource(constants_.Get(), 0, nullptr, &parameters, 0, 0);
    draw(photoreal_pixel_.Get(), input, resources_.stage_output(),
        neural_residual_.Get(), lut_.view());
    draw(finish_pixel_.Get(), resources_.stage_input(), resources_.output());
    frame_index_ = frame_index_ >= 4095.0f ? 0.0f : frame_index_ + 1.0f;
}
}
