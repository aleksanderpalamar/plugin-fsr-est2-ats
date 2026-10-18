#include "raytracing/gpu_readback.hpp"

#include <array>
#include <cstring>
#include <wrl/client.h>

namespace neuralfx::raytracing {
namespace {
using Texel = std::array<float, 4>;

Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_copy(ID3D11Device* device,
    ID3D11DeviceContext* context, ID3D11Texture2D* texture) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    description.BindFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&description, nullptr, &staging))) return nullptr;
    context->CopyResource(staging.Get(), texture);
    return staging;
}

Vec3 to_vec3(const unsigned char* source) noexcept {
    Texel texel{};
    std::memcpy(texel.data(), source, sizeof(Texel));
    return {texel[0], texel[1], texel[2]};
}
}

std::optional<Image> read_image(ID3D11Device* device, ID3D11DeviceContext* context,
    ID3D11Texture2D* texture, int width, int height) {
    if (!device || !context || !texture || width <= 0 || height <= 0) return std::nullopt;
    auto staging = staging_copy(device, context, texture);
    if (!staging) return std::nullopt;
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    size_t columns = static_cast<size_t>(width);
    Image image{width, height, std::vector<Vec3>(columns * static_cast<size_t>(height))};
    for (size_t row = 0; row < static_cast<size_t>(height); ++row) {
        const auto* line = static_cast<const unsigned char*>(mapped.pData) + row * mapped.RowPitch;
        for (size_t column = 0; column < columns; ++column)
            image.pixels[row * columns + column] = to_vec3(line + column * sizeof(Texel));
    }
    context->Unmap(staging.Get(), 0);
    return image;
}
}
