#include "screenshots/d3d11_readback.hpp"
#include "common/logger.hpp"

#include <cstdio>

namespace neuralfx {
namespace {
std::optional<ScreenshotPixelFormat> pixel_format(DXGI_FORMAT format) noexcept {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return ScreenshotPixelFormat::Rgba8;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return ScreenshotPixelFormat::Bgra8;
    default:
        return std::nullopt;
    }
}

void log_read_error(const char* operation, HRESULT result) noexcept {
    char message[128]{};
    std::snprintf(message, sizeof(message), "Steam screenshot %s failed: HRESULT=0x%08lx",
        operation, static_cast<unsigned long>(result));
    log(message);
}

struct MappedTexture {
    ID3D11DeviceContext* context;
    ID3D11Texture2D* texture;
    ~MappedTexture() { context->Unmap(texture, 0); }
};
}

HRESULT D3D11ScreenshotReadback::prepare(ID3D11Texture2D* texture) noexcept {
    if (!texture) return E_POINTER;
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    if (!pixel_format(description.Format) || description.SampleDesc.Count != 1 ||
        description.ArraySize != 1 || description.MipLevels != 1) return DXGI_ERROR_UNSUPPORTED;
    if (!screenshot_rgb_size(description.Width, description.Height)) return E_INVALIDARG;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    texture->GetDevice(&device);
    if (!device) return E_FAIL;
    if (staging_ && device_ == device && description_.Width == description.Width &&
        description_.Height == description.Height && description_.Format == description.Format) return S_OK;
    reset();
    device_ = device;
    device_->GetImmediateContext(&context_);
    if (!context_) return E_FAIL;
    description.Usage = D3D11_USAGE_STAGING;
    description.BindFlags = 0;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    HRESULT result = device_->CreateTexture2D(&description, nullptr, &staging_);
    if (FAILED(result)) return result;
    description_ = description;
    return S_OK;
}

std::optional<RgbImage> D3D11ScreenshotReadback::capture(ID3D11Texture2D* texture) {
    HRESULT result = prepare(texture);
    if (FAILED(result)) {
        log_read_error("prepare", result);
        return std::nullopt;
    }
    auto format = pixel_format(description_.Format);
    if (!format) return std::nullopt;
    context_->CopyResource(staging_.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    result = context_->Map(staging_.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(result)) {
        log_read_error("Map", result);
        log_read_error("device status", device_->GetDeviceRemovedReason());
        return std::nullopt;
    }
    MappedTexture guard{context_.Get(), staging_.Get()};
    if (!mapped.pData) return std::nullopt;
    auto size = static_cast<std::size_t>(mapped.RowPitch) * description_.Height;
    auto image = screenshot_rgb({static_cast<const std::uint8_t*>(mapped.pData), size},
        description_.Width, description_.Height, mapped.RowPitch, *format);
    if (!image) log("Steam screenshot RGB conversion rejected the mapped buffer");
    return image;
}

void D3D11ScreenshotReadback::reset() noexcept {
    staging_.Reset();
    context_.Reset();
    device_.Reset();
    description_ = {};
}
}
