#include "renderer/resource_manager.hpp"

namespace neuralfx {
namespace {
bool supported(DXGI_FORMAT format) noexcept {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return true;
    default:
        return false;
    }
}
}

HRESULT RenderResourceManager::initialize(ID3D11Device* device, IDXGISwapChain* swapchain) noexcept {
    reset();
    if (!device || !swapchain) return E_POINTER;
    HRESULT result = swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer_));
    if (FAILED(result)) return result;
    D3D11_TEXTURE2D_DESC description{};
    backbuffer_->GetDesc(&description);
    if (!supported(description.Format) || description.SampleDesc.Count != 1) return DXGI_ERROR_UNSUPPORTED;
    if (description.Width == 0 || description.Height == 0) return E_INVALIDARG;
    result = device->CreateRenderTargetView(backbuffer_.Get(), nullptr, &output_);
    if (FAILED(result)) return result;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.CPUAccessFlags = 0;
    description.MiscFlags = 0;
    result = device->CreateTexture2D(&description, nullptr, &copy_);
    if (FAILED(result)) return result;
    result = device->CreateShaderResourceView(copy_.Get(), nullptr, &input_);
    if (FAILED(result)) return result;
    width_ = description.Width;
    height_ = description.Height;
    format_ = description.Format;
    return S_OK;
}

void RenderResourceManager::reset() noexcept {
    input_.Reset();
    output_.Reset();
    copy_.Reset();
    backbuffer_.Reset();
    width_ = 0;
    height_ = 0;
    format_ = DXGI_FORMAT_UNKNOWN;
}
}
