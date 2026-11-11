#include "renderer/upscale_inputs.hpp"
#include "renderer/context_state.hpp"

namespace neuralfx {
namespace {
using Microsoft::WRL::ComPtr;

bool compatible_color(ID3D11Texture2D* color, temporal::RenderSize render) {
    D3D11_TEXTURE2D_DESC description{};
    color->GetDesc(&description);
    bool family = description.Format == DXGI_FORMAT_R8G8B8A8_UNORM || description.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
        description.Format == DXGI_FORMAT_R8G8B8A8_TYPELESS;
    return family && description.Width == render.width && description.Height == render.height &&
        description.SampleDesc.Count == 1;
}

ComPtr<ID3D11Texture2D> make_texture(ID3D11Device* device, DXGI_FORMAT format, temporal::RenderSize size, UINT bind) {
    D3D11_TEXTURE2D_DESC description{size.width, size.height, 1, 1, format, {1, 0}, D3D11_USAGE_DEFAULT, bind, 0, 0};
    ComPtr<ID3D11Texture2D> texture;
    device->CreateTexture2D(&description, nullptr, &texture);
    return texture;
}
}

HRESULT UpscaleInputs::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    HRESULT result = resolve_frame_.initialize(device, shader_directory);
    if (SUCCEEDED(result)) result = motion_.initialize(device, shader_directory);
    if (SUCCEEDED(result)) result = depth_pass_.initialize(device, shader_directory);
    return result;
}

bool UpscaleInputs::ensure(ID3D11Device* device, const UpscaleSources& sources) noexcept {
    bool same = color_ && render_.width == sources.render.width && render_.height == sources.render.height &&
        output_size_.width == sources.output.width && output_size_.height == sources.output.height;
    if (same) return true;
    color_ = make_texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, sources.render, D3D11_BIND_SHADER_RESOURCE);
    depth_ = make_texture(device, DXGI_FORMAT_R32_FLOAT, sources.render,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    output_ = make_texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, sources.output,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    depth_target_.Reset();
    output_view_.Reset();
    bool created = color_ && depth_ && output_ && SUCCEEDED(device->CreateRenderTargetView(depth_.Get(), nullptr, &depth_target_)) &&
        SUCCEEDED(device->CreateShaderResourceView(output_.Get(), nullptr, &output_view_));
    if (created) {
        render_ = sources.render;
        output_size_ = sources.output;
    } else {
        color_.Reset();
    }
    return created;
}

std::optional<upscale::FrameResources> UpscaleInputs::prepare(ID3D11DeviceContext* context,
    const UpscaleSources& sources) noexcept {
    if (!context || !sources.color || !sources.scene_depth || !sources.resolve) return std::nullopt;
    if (!compatible_color(sources.color, sources.render)) return std::nullopt;
    ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    if (!device || !ensure(device.Get(), sources)) return std::nullopt;
    ContextState saved(context);
    if (!targets_.prepare(device.Get(), sources.scene_depth, sources.render)) return std::nullopt;
    targets_.copy_depth(context, sources.scene_depth);
    auto* camera = resolve_frame_.run(context, sources.resolve, sources.render);
    if (!camera) return std::nullopt;
    motion_.render(context, {targets_.depth(), camera, targets_.motion_target()}, sources.render);
    depth_pass_.render(context, {targets_.depth(), depth_target_.Get()}, sources.render);
    context->CopyResource(color_.Get(), sources.color);
    return upscale::FrameResources{color_.Get(), depth_.Get(), targets_.motion_texture(), output_.Get()};
}
}
