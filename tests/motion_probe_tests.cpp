#include "common/logger.hpp"
#include "diagnostics/motion_probe.hpp"
#include "dxgi/context_hook.hpp"
#include "scene_frame_fixture.hpp"

#include <cstdlib>
#include <fstream>

using namespace scene_test;
namespace diagnostics = neuralfx::diagnostics;

namespace {
constexpr CameraValues captured_camera{-0.47095215f, 0.0f, -0.8821588f, -46.862404f,
    0.0f, 1.0f, 0.0f, 10.617724f,
    0.8821588f, 0.0f, -0.47095215f, -48.97045f,
    0.0f, 0.0f, 0.0f, 1.0f,
    -0.5872703f, 0.0f, -1.1014271f, -58.526123f,
    -0.0006845583f, 2.2190397f, 0.0003654605f, 23.599154f,
    0.00011026186f, 0.0f, -5.8864753e-05f, 0.19390413f,
    -0.8821588f, 0.0f, 0.47095215f, 48.97045f};

void request_toggle() {
    std::ofstream(neuralfx::module_directory() / L"NeuralFX" / L"motion-debug").put('\n');
}

void present(Device& gpu, const Targets& targets, ID3D11RenderTargetView* output) {
    const float black[4]{0.0f, 0.0f, 0.0f, 1.0f};
    gpu.context->ClearRenderTargetView(output, black);
    gpu.context->ClearDepthStencilView(targets.depth_view.Get(), D3D11_CLEAR_DEPTH, 0.5f, 0);
    game_frame(gpu, targets, true);
    diagnostics::present_motion_probe(gpu.swapchain.Get());
    diagnostics::motion_probe_frame_end(gpu.swapchain.Get());
}

std::optional<std::array<std::uint8_t, 4>> pixel(Device& gpu, ID3D11Texture2D* texture, UINT x, UINT y) {
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
    std::array<std::uint8_t, 4> value{};
    std::memcpy(value.data(), static_cast<const std::uint8_t*>(mapped.pData) + y * mapped.RowPitch + x * 4, 4);
    gpu.context->Unmap(staging.Get(), 0);
    return value;
}

int check(Device& gpu, ID3D11Texture2D* backbuffer) {
    auto inside = pixel(gpu, backbuffer, 4, 4);
    auto outside = pixel(gpu, backbuffer, 24, 24);
    if (!inside || !outside) return 20;
    for (int channel = 0; channel < 3; ++channel) {
        if (std::abs((*inside)[channel] - 128) > 2) return 21;
        if ((*outside)[channel] != 0) return 22;
    }
    return 0;
}
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW, 0, 0, 32, 32,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    Device gpu;
    Pipeline pipeline;
    Targets targets;
    if (!window || FAILED(create_device(window, gpu))) return 1;
    if (!create_pipeline(gpu.device.Get(), pipeline) || !create_targets(gpu.device.Get(), captured_camera, targets))
        return 2;
    bind_pipeline(gpu.context.Get(), pipeline);
    neuralfx::hook_context(gpu.swapchain.Get());
    ComPtr<ID3D11Texture2D> backbuffer;
    ComPtr<ID3D11RenderTargetView> output;
    gpu.swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    if (FAILED(gpu.device->CreateRenderTargetView(backbuffer.Get(), nullptr, &output))) return 3;
    request_toggle();
    diagnostics::motion_probe_frame_end(gpu.swapchain.Get());
    present(gpu, targets, output.Get());
    present(gpu, targets, output.Get());
    int result = check(gpu, backbuffer.Get());
    DestroyWindow(window);
    return result;
}
