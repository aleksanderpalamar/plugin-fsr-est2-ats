#include "common/logger.hpp"
#include "d3d11_test_device.hpp"
#include "renderer/blit_pass.hpp"

using namespace neuralfx;
using namespace neuralfx::testing;

namespace {
constexpr UINT width = 8;
constexpr UINT height = 4;
}

int srgb_target_case(TestDevice& gpu, BlitPass& pass) {
    std::vector<unsigned char> gray(width * height * 4, 128);
    auto source = create_input(gpu.device.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, gray.data(), width * 4);
    auto description = texture_description(width, height, DXGI_FORMAT_B8G8R8A8_TYPELESS, D3D11_BIND_RENDER_TARGET);
    ComPtr<ID3D11Texture2D> target;
    ComPtr<ID3D11RenderTargetView> view;
    D3D11_RENDER_TARGET_VIEW_DESC view_description{DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, D3D11_RTV_DIMENSION_TEXTURE2D, {}};
    if (!source || FAILED(gpu.device->CreateTexture2D(&description, nullptr, &target))) return 20;
    if (FAILED(gpu.device->CreateRenderTargetView(target.Get(), &view_description, &view))) return 21;
    pass.render(gpu.context.Get(), source.Get(), view.Get(), {width, height});
    description.BindFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(gpu.device->CreateTexture2D(&description, nullptr, &staging))) return 22;
    gpu.context->CopyResource(staging.Get(), target.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return 23;
    const auto* bytes = static_cast<const unsigned char*>(mapped.pData);
    bool unchanged = bytes[0] == 128 && bytes[1] == 128 && bytes[2] == 128;
    gpu.context->Unmap(staging.Get(), 0);
    return unchanged ? 0 : 24;
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) return 1;
    BlitPass pass;
    if (FAILED(pass.initialize(gpu->device.Get(), module_directory() / L"NeuralFX" / L"shaders"))) return 2;
    std::vector<unsigned char> pixels(width * height * 4);
    for (size_t index = 0; index < pixels.size(); index += 4) {
        pixels[index] = 10;
        pixels[index + 1] = 20;
        pixels[index + 2] = 30;
        pixels[index + 3] = 255;
    }
    auto source = create_input(gpu->device.Get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, pixels.data(), width * 4);
    auto description = texture_description(width, height, DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ComPtr<ID3D11Texture2D> target;
    ComPtr<ID3D11RenderTargetView> view;
    if (!source || FAILED(gpu->device->CreateTexture2D(&description, nullptr, &target))) return 3;
    if (FAILED(gpu->device->CreateRenderTargetView(target.Get(), nullptr, &view))) return 4;
    pass.render(gpu->context.Get(), source.Get(), view.Get(), {width, height});
    description.BindFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(gpu->device->CreateTexture2D(&description, nullptr, &staging))) return 5;
    gpu->context->CopyResource(staging.Get(), target.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu->context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return 6;
    const auto* bytes = static_cast<const unsigned char*>(mapped.pData);
    bool swizzled = bytes[0] == 30 && bytes[1] == 20 && bytes[2] == 10 && bytes[3] == 255;
    gpu->context->Unmap(staging.Get(), 0);
    pass.render(gpu->context.Get(), nullptr, view.Get(), {width, height});
    if (!swizzled) return 7;
    return srgb_target_case(*gpu, pass);
}
