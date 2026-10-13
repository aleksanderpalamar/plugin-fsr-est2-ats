#include "screenshots/d3d11_readback.hpp"

#include <array>
#include <cstdio>

namespace {
using Microsoft::WRL::ComPtr;
bool exercise(ID3D11Device* device, ID3D11DeviceContext* context, DXGI_FORMAT format) {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = 2;
    description.Height = 2;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.SampleDesc.Count = 1;
    description.Format = format;
    description.BindFlags = D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&description, nullptr, &texture))) return false;
    neuralfx::D3D11ScreenshotReadback readback;
    if (FAILED(readback.prepare(texture.Get()))) return false;
    std::array<std::uint8_t, 16> rgba{128, 64, 32, 5, 0, 255, 0, 7,
        0, 0, 255, 9, 255, 255, 255, 11};
    bool bgra = format == DXGI_FORMAT_B8G8R8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    if (bgra) std::swap(rgba[0], rgba[2]);
    if (bgra) std::swap(rgba[8], rgba[10]);
    context->UpdateSubresource(texture.Get(), 0, nullptr, rgba.data(), 8, 0);
    auto captured = readback.capture(texture.Get());
    std::vector<std::uint8_t> expected{128, 64, 32, 0, 255, 0, 0, 0, 255, 255, 255, 255};
    if (!captured || captured->pixels != expected) return false;
    ComPtr<ID3D11RenderTargetView> target;
    if (FAILED(device->CreateRenderTargetView(texture.Get(), nullptr, &target))) return false;
    float final_color[4]{0, 1, 0, 1};
    context->ClearRenderTargetView(target.Get(), final_color);
    captured = readback.capture(texture.Get());
    if (!captured || captured->pixels != std::vector<std::uint8_t>{0, 255, 0, 0, 255, 0,
        0, 255, 0, 0, 255, 0}) return false;
    readback.reset();
    description.Width = 3;
    description.Height = 1;
    texture.Reset();
    if (FAILED(device->CreateTexture2D(&description, nullptr, &texture))) return false;
    if (FAILED(readback.prepare(texture.Get()))) return false;
    description.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texture.Reset();
    if (FAILED(device->CreateTexture2D(&description, nullptr, &texture))) return false;
    return readback.prepare(texture.Get()) == DXGI_ERROR_UNSUPPORTED;
}
}

int main() {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
    if (FAILED(result)) result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
    if (FAILED(result)) {
        std::fprintf(stderr, "D3D11 unavailable: 0x%08lx\n", static_cast<unsigned long>(result));
        return 1;
    }
    if (!exercise(device.Get(), context.Get(), DXGI_FORMAT_R8G8B8A8_UNORM)) return 2;
    if (!exercise(device.Get(), context.Get(), DXGI_FORMAT_B8G8R8A8_UNORM)) return 3;
    if (!exercise(device.Get(), context.Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)) return 4;
    if (!exercise(device.Get(), context.Get(), DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)) return 5;
    return 0;
}
