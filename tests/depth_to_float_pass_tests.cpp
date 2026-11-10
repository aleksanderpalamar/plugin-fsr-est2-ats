#include "common/logger.hpp"
#include "d3d11_test_device.hpp"
#include "renderer/depth_to_float_pass.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx;
using namespace neuralfx::testing;

namespace {
constexpr UINT width = 16;
constexpr UINT height = 8;

float depth_at(UINT x, UINT y) {
    return static_cast<float>(x + y * width) / static_cast<float>(width * height);
}

ComPtr<ID3D11ShaderResourceView> depth_source(ID3D11Device* device) {
    std::vector<float> values(width * height);
    for (UINT y = 0; y < height; ++y)
        for (UINT x = 0; x < width; ++x) values[x + y * width] = depth_at(x, y);
    auto description = texture_description(width, height, DXGI_FORMAT_R32_TYPELESS, D3D11_BIND_SHADER_RESOURCE);
    D3D11_SUBRESOURCE_DATA data{values.data(), width * sizeof(float), 0};
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&description, &data, &texture))) return nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC view{DXGI_FORMAT_R32_FLOAT, D3D11_SRV_DIMENSION_TEXTURE2D, {}};
    view.Texture2D.MipLevels = 1;
    ComPtr<ID3D11ShaderResourceView> result;
    device->CreateShaderResourceView(texture.Get(), &view, &result);
    return result;
}

struct Target {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11RenderTargetView> view;
};

std::optional<Target> make_target(ID3D11Device* device) {
    auto description = texture_description(width, height, DXGI_FORMAT_R32_FLOAT, D3D11_BIND_RENDER_TARGET);
    Target target;
    if (FAILED(device->CreateTexture2D(&description, nullptr, &target.texture))) return std::nullopt;
    if (FAILED(device->CreateRenderTargetView(target.texture.Get(), nullptr, &target.view))) return std::nullopt;
    return target;
}

std::optional<std::vector<float>> read_back(const TestDevice& gpu, ID3D11Texture2D* texture) {
    auto description = texture_description(width, height, DXGI_FORMAT_R32_FLOAT, 0);
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(gpu.device->CreateTexture2D(&description, nullptr, &staging))) return std::nullopt;
    gpu.context->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    std::vector<float> values(width * height);
    for (UINT row = 0; row < height; ++row)
        std::memcpy(values.data() + row * width, static_cast<const unsigned char*>(mapped.pData) + row * mapped.RowPitch,
            width * sizeof(float));
    gpu.context->Unmap(staging.Get(), 0);
    return values;
}
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) return 1;
    DepthToFloatPass pass;
    if (FAILED(pass.initialize(gpu->device.Get(), module_directory() / L"NeuralFX" / L"shaders"))) return 2;
    auto source = depth_source(gpu->device.Get());
    auto target = make_target(gpu->device.Get());
    if (!source || !target) return 3;
    pass.render(gpu->context.Get(), {source.Get(), target->view.Get()}, {width, height});
    auto values = read_back(*gpu, target->texture.Get());
    if (!values) return 4;
    for (UINT y = 0; y < height; ++y)
        for (UINT x = 0; x < width; ++x)
            if (std::fabs((*values)[x + y * width] - depth_at(x, y)) > 1e-6f) return 5;
    pass.render(gpu->context.Get(), {nullptr, target->view.Get()}, {width, height});
    return 0;
}
