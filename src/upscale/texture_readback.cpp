#include "upscale/texture_readback.hpp"

#include <wrl/client.h>

#include <cstring>

namespace neuralfx::upscale {
namespace {
using Microsoft::WRL::ComPtr;

std::size_t texel_bytes(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT_R8_UNORM) return 1;
    if (format == DXGI_FORMAT_R32G8X24_TYPELESS || format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) return 8;
    return 4;
}
}

std::optional<Readback> read_texture(ID3D11DeviceContext* context, ID3D11Texture2D* texture) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    description.BindFlags = 0;
    description.MiscFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&description, nullptr, &staging))) return std::nullopt;
    context->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    Readback result{description.Width, description.Height, {}};
    std::size_t row = static_cast<std::size_t>(description.Width) * texel_bytes(description.Format);
    result.bytes.resize(row * description.Height);
    for (UINT y = 0; y < description.Height; ++y)
        std::memcpy(result.bytes.data() + y * row, static_cast<const std::uint8_t*>(mapped.pData) + y * mapped.RowPitch, row);
    context->Unmap(staging.Get(), 0);
    return result;
}

std::optional<std::vector<std::uint8_t>> read_buffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer) {
    D3D11_BUFFER_DESC description{};
    buffer->GetDesc(&description);
    description.BindFlags = 0;
    description.MiscFlags = 0;
    description.StructureByteStride = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    ComPtr<ID3D11Buffer> staging;
    if (FAILED(device->CreateBuffer(&description, nullptr, &staging))) return std::nullopt;
    context->CopyResource(staging.Get(), buffer);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    std::vector<std::uint8_t> bytes(description.ByteWidth);
    std::memcpy(bytes.data(), mapped.pData, bytes.size());
    context->Unmap(staging.Get(), 0);
    return bytes;
}
}
