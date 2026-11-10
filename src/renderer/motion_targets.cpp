#include "renderer/motion_targets.hpp"
#include "renderer/depth_format.hpp"

namespace neuralfx {
namespace {
using Microsoft::WRL::ComPtr;

bool same_source(const D3D11_TEXTURE2D_DESC& left, const D3D11_TEXTURE2D_DESC& right) {
    return left.Width == right.Width && left.Height == right.Height && left.Format == right.Format;
}

D3D11_TEXTURE2D_DESC texture_description(UINT width, UINT height, DXGI_FORMAT format, UINT bind) {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = format;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = bind;
    return description;
}

D3D11_SHADER_RESOURCE_VIEW_DESC view_description(DXGI_FORMAT format) {
    D3D11_SHADER_RESOURCE_VIEW_DESC description{};
    description.Format = format;
    description.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    description.Texture2D.MipLevels = 1;
    return description;
}
}

bool MotionTargets::prepare(ID3D11Device* device, ID3D11Texture2D* scene_depth,
    temporal::RenderSize size) noexcept {
    if (!device || !scene_depth) return false;
    D3D11_TEXTURE2D_DESC source{};
    scene_depth->GetDesc(&source);
    if (!depth_view_ || !same_source(source, source_)) {
        if (!create_depth(device, source)) return false;
    }
    if (!motion_view_ || size.width != size_.width || size.height != size_.height) {
        if (!create_motion(device, size)) return false;
    }
    return true;
}

void MotionTargets::copy_depth(ID3D11DeviceContext* context, ID3D11Texture2D* scene_depth) noexcept {
    if (context && scene_depth && depth_) context->CopyResource(depth_.Get(), scene_depth);
}

bool MotionTargets::create_depth(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& source) noexcept {
    depth_view_.Reset();
    depth_.Reset();
    auto formats = depth_format(source.Format);
    if (!formats || source.SampleDesc.Count != 1) return false;
    auto description = texture_description(source.Width, source.Height, formats->texture, D3D11_BIND_SHADER_RESOURCE);
    if (FAILED(device->CreateTexture2D(&description, nullptr, &depth_))) return false;
    auto view = view_description(formats->view);
    if (FAILED(device->CreateShaderResourceView(depth_.Get(), &view, &depth_view_))) return false;
    source_ = source;
    return true;
}

bool MotionTargets::create_motion(ID3D11Device* device, temporal::RenderSize size) noexcept {
    motion_view_.Reset();
    motion_target_.Reset();
    auto description = texture_description(size.width, size.height, DXGI_FORMAT_R16G16_FLOAT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    motion_.Reset();
    if (FAILED(device->CreateTexture2D(&description, nullptr, &motion_))) return false;
    if (FAILED(device->CreateRenderTargetView(motion_.Get(), nullptr, &motion_target_))) return false;
    if (FAILED(device->CreateShaderResourceView(motion_.Get(), nullptr, &motion_view_))) return false;
    size_ = size;
    return true;
}
}
