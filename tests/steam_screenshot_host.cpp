#include "steam_screenshot_host_api.hpp"

#include <d3d11.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstdlib>

namespace {
using Microsoft::WRL::ComPtr;
int game_requests = 0;
struct GameEvents final : neuralfx::ScreenshotEvents {
    void requested() noexcept override {
        ++game_requests;
        std::uint8_t original[3]{1, 2, 3};
        SteamAPI_ISteamScreenshots_WriteScreenshot(
            SteamAPI_SteamScreenshots_v003(), original, 3, 1, 1);
    }
    void ready(std::uint32_t, int) noexcept override {}
};

HWND window() {
    WNDCLASSW type{};
    type.lpfnWndProc = DefWindowProcW;
    type.hInstance = GetModuleHandleW(nullptr);
    type.lpszClassName = L"NeuralFXScreenshotHost";
    if (!RegisterClassW(&type)) return nullptr;
    HWND result = CreateWindowW(type.lpszClassName, L"NeuralFX screenshot regression",
        WS_OVERLAPPEDWINDOW, 0, 0, 320, 240, nullptr, nullptr, type.hInstance, nullptr);
    if (!result) return nullptr;
    ShowWindow(result, SW_SHOW);
    SetForegroundWindow(result);
    return result;
}

bool final_frame(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* chain) {
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(chain->GetBuffer(0, IID_PPV_ARGS(&texture)))) return false;
    ComPtr<ID3D11RenderTargetView> target;
    if (FAILED(device->CreateRenderTargetView(texture.Get(), nullptr, &target))) return false;
    float color[]{0.05f, 0.1f, 0.15f, 1};
    context->ClearRenderTargetView(target.Get(), color);
    return SUCCEEDED(chain->Present(0, 0));
}

int exercise(HWND output) {
    GameEvents events;
    neuralfx::SteamScreenshotCallback game{2302};
    game.listen(&events);
    SteamAPI_RegisterCallback(&game, 2302);
    SteamAPI_ISteamScreenshots_HookScreenshots(SteamAPI_SteamScreenshots_v003(), true);
    static ComPtr<IDXGIFactory> factory;
    if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(&factory)))) return 2;
    static ComPtr<ID3D11Device> device;
    static ComPtr<ID3D11DeviceContext> context;
    HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
    if (FAILED(result)) result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
    if (FAILED(result)) return 3;
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferDesc.Width = 64;
    description.BufferDesc.Height = 48;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 1;
    description.OutputWindow = output;
    description.Windowed = TRUE;
    static ComPtr<IDXGISwapChain> chain;
    if (FAILED(factory->CreateSwapChain(device.Get(), &description, &chain))) return 4;
    if (!final_frame(device.Get(), context.Get(), chain.Get())) return 5;
    if (ScreenshotTest_Writes() != 0) return 6;
    ScreenshotTest_Request();
    SteamAPI_RunCallbacks();
    if (!final_frame(device.Get(), context.Get(), chain.Get())) return 7;
    std::fprintf(stderr, "game callbacks=%d writes=%d RGB=%d,%d,%d dimensions=%dx%d\n",
        game_requests, ScreenshotTest_Writes(), ScreenshotTest_Channel(0), ScreenshotTest_Channel(1),
        ScreenshotTest_Channel(2), ScreenshotTest_Width(), ScreenshotTest_Height());
    if (game_requests != 0 || ScreenshotTest_Writes() != 1) return 8;
    if (ScreenshotTest_Width() != 64 || ScreenshotTest_Height() != 48) return 9;
    int expected[]{64, 128, 191};
    for (int channel = 0; channel < 3; ++channel)
        if (std::abs(ScreenshotTest_Channel(channel) - expected[channel]) > 2) return 10;
    SteamAPI_UnregisterCallback(&game);
    if (game.registered()) return 11;
    return 0;
}
}

int main() {
    auto output = window();
    if (!output) return 1;
    int result = exercise(output);
    DestroyWindow(output);
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(result));
    return result;
}
