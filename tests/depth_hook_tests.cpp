#include "dxgi/context_hook.hpp"
#include "dxgi/depth_capture.hpp"

#include <d3d11.h>
#include <windows.h>
#include <wrl/client.h>
#include <cstdio>

namespace {
using Microsoft::WRL::ComPtr;

HRESULT create_device(const DXGI_SWAP_CHAIN_DESC& description,
    ComPtr<IDXGISwapChain>& swapchain, ComPtr<ID3D11Device>& device,
    ComPtr<ID3D11DeviceContext>& context) {
    auto create = [&](D3D_DRIVER_TYPE type) {
        return D3D11CreateDeviceAndSwapChain(nullptr, type, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &description, &swapchain, &device, nullptr, &context);
    };
    HRESULT result = create(D3D_DRIVER_TYPE_HARDWARE);
    if (SUCCEEDED(result)) return result;
    swapchain.Reset();
    device.Reset();
    context.Reset();
    return create(D3D_DRIVER_TYPE_WARP);
}

HRESULT create_depth(ID3D11Device* device, ComPtr<ID3D11DepthStencilView>& view) {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = 32;
    description.Height = 32;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R24G8_TYPELESS;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ComPtr<ID3D11Texture2D> texture;
    HRESULT result = device->CreateTexture2D(&description, nullptr, &texture);
    if (FAILED(result)) return result;
    D3D11_DEPTH_STENCIL_VIEW_DESC view_description{};
    view_description.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    view_description.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    return device->CreateDepthStencilView(texture.Get(), &view_description, &view);
}
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW,
        0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return 1;
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferDesc.Width = 32;
    description.BufferDesc.Height = 32;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.OutputWindow = window;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    ComPtr<IDXGISwapChain> swapchain;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    HRESULT result = create_device(description, swapchain, device, context);
    if (FAILED(result)) {
        std::fprintf(stderr, "D3D11 device creation failed: 0x%08lx\n",
            static_cast<unsigned long>(result));
        return 2;
    }
    ComPtr<ID3D11DepthStencilView> depth;
    if (FAILED(create_depth(device.Get(), depth))) return 3;
    neuralfx::DepthCapture::instance().configure(device.Get(), 32, 32);
    neuralfx::DepthCapture::instance().set_active(true);
    neuralfx::hook_context(swapchain.Get());
    context->OMSetRenderTargets(0, nullptr, depth.Get());
    context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr,
        0, 0, nullptr, nullptr);
    if (!neuralfx::DepthCapture::instance().take_view()) return 4;
    context->OMSetRenderTargets(0, nullptr, depth.Get());
    context->OMSetRenderTargetsAndUnorderedAccessViews(
        D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
        0, 0, nullptr, nullptr);
    if (neuralfx::DepthCapture::instance().take_view()) return 5;
    neuralfx::DepthCapture::instance().set_active(false);
    context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr,
        0, 0, nullptr, nullptr);
    if (neuralfx::DepthCapture::instance().take_view()) return 6;
    ComPtr<ID3D11DepthStencilView> bound;
    context->OMGetRenderTargets(0, nullptr, &bound);
    if (bound) return 7;
    neuralfx::DepthCapture::instance().reset();
    DestroyWindow(window);
    return 0;
}
