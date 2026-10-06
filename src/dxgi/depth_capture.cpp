#include "dxgi/depth_capture.hpp"
#include "renderer/depth_format.hpp"
#include "common/logger.hpp"

namespace neuralfx {
DepthCapture& DepthCapture::instance() noexcept {
    static DepthCapture capture;
    return capture;
}

void DepthCapture::configure(ID3D11Device* device, UINT width, UINT height) noexcept {
    std::lock_guard lock(mutex_);
    if (device_.Get() == device && width_ == width && height_ == height) return;
    copy_.Reset();
    view_.Reset();
    device_ = device;
    width_ = width;
    height_ = height;
    format_ = DXGI_FORMAT_UNKNOWN;
    ready_ = false;
}

void DepthCapture::reset() noexcept {
    set_active(false);
    configure(nullptr, 0, 0);
}

bool DepthCapture::prepare(const D3D11_TEXTURE2D_DESC& source,
    DXGI_FORMAT texture_format, DXGI_FORMAT view_format) noexcept {
    if (copy_ && format_ == source.Format) return true;
    copy_.Reset();
    view_.Reset();
    D3D11_TEXTURE2D_DESC description = source;
    description.Format = texture_format;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.CPUAccessFlags = 0;
    description.MiscFlags = 0;
    HRESULT result = device_->CreateTexture2D(&description, nullptr, &copy_);
    if (FAILED(result)) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC view_description{};
    view_description.Format = view_format;
    view_description.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    view_description.Texture2D.MipLevels = 1;
    result = device_->CreateShaderResourceView(copy_.Get(), &view_description, &view_);
    if (FAILED(result)) {
        copy_.Reset();
        return false;
    }
    format_ = source.Format;
    log("Screen-space depth capture ready");
    return true;
}

void DepthCapture::capture(ID3D11DeviceContext* context, ID3D11DepthStencilView* depth) noexcept {
    if (!active() || !context || !depth) return;
    Microsoft::WRL::ComPtr<ID3D11Resource> resource;
    depth->GetResource(&resource);
    if (!resource) return;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    if (FAILED(resource.As(&texture))) return;
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    D3D11_DEPTH_STENCIL_VIEW_DESC depth_description{};
    depth->GetDesc(&depth_description);
    DXGI_FORMAT source_format = depth_description.Format == DXGI_FORMAT_UNKNOWN
        ? description.Format : depth_description.Format;
    auto formats = depth_format(source_format);
    if (!formats) return;
    if (description.MipLevels != 1 || description.ArraySize != 1) return;
    if (description.SampleDesc.Count != 1) return;
    std::lock_guard lock(mutex_);
    if (!device_ || description.Width != width_ || description.Height != height_) return;
    Microsoft::WRL::ComPtr<ID3D11Device> owner;
    texture->GetDevice(&owner);
    if (owner.Get() != device_.Get()) return;
    if (!prepare(description, formats->texture, formats->view)) return;
    context->CopyResource(copy_.Get(), texture.Get());
    ready_ = true;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> DepthCapture::take_view() noexcept {
    std::lock_guard lock(mutex_);
    if (!ready_) return nullptr;
    ready_ = false;
    return view_;
}
}
