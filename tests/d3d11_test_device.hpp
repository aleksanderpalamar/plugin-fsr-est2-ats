#pragma once

#include <d3d11.h>
#include <array>
#include <cstring>
#include <optional>
#include <vector>
#include <wrl/client.h>

namespace neuralfx::testing {
using Microsoft::WRL::ComPtr;
using Pixel = std::array<float, 4>;

struct TestDevice {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
};

struct FloatTarget {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11RenderTargetView> view;
};

inline std::optional<TestDevice> create_test_device() {
    for (D3D_DRIVER_TYPE type : {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP}) {
        TestDevice result;
        if (SUCCEEDED(D3D11CreateDevice(nullptr, type, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &result.device, nullptr, &result.context))) return result;
    }
    return std::nullopt;
}

inline D3D11_TEXTURE2D_DESC texture_description(UINT width, UINT height,
    DXGI_FORMAT format, UINT bind_flags) {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = format;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = bind_flags;
    return description;
}

inline ComPtr<ID3D11ShaderResourceView> create_input(ID3D11Device* device, UINT width,
    UINT height, DXGI_FORMAT format, const void* data, UINT pitch) {
    auto description = texture_description(width, height, format, D3D11_BIND_SHADER_RESOURCE);
    D3D11_SUBRESOURCE_DATA initial{data, pitch, 0};
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&description, &initial, &texture))) return nullptr;
    ComPtr<ID3D11ShaderResourceView> view;
    if (FAILED(device->CreateShaderResourceView(texture.Get(), nullptr, &view))) return nullptr;
    return view;
}

inline std::optional<FloatTarget> create_float_target(ID3D11Device* device, UINT width, UINT height) {
    auto description = texture_description(width, height, DXGI_FORMAT_R32G32B32A32_FLOAT,
        D3D11_BIND_RENDER_TARGET);
    FloatTarget target;
    if (FAILED(device->CreateTexture2D(&description, nullptr, &target.texture))) return std::nullopt;
    if (FAILED(device->CreateRenderTargetView(target.texture.Get(), nullptr, &target.view)))
        return std::nullopt;
    return target;
}

inline std::optional<std::vector<Pixel>> read_pixels(const TestDevice& gpu,
    ID3D11Texture2D* texture, UINT width, UINT height) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    description.BindFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(gpu.device->CreateTexture2D(&description, nullptr, &staging))) return std::nullopt;
    gpu.context->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    std::vector<Pixel> pixels(static_cast<size_t>(width) * height);
    for (UINT row = 0; row < height; ++row) {
        const auto* source = static_cast<const unsigned char*>(mapped.pData) + row * mapped.RowPitch;
        std::memcpy(pixels.data() + static_cast<size_t>(row) * width, source, width * sizeof(Pixel));
    }
    gpu.context->Unmap(staging.Get(), 0);
    return pixels;
}
}
