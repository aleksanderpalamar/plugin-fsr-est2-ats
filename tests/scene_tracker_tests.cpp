#include "dxgi/context_hook.hpp"
#include "frame_trace_hook_fixture.hpp"
#include "temporal/scene_tracker.hpp"

#include <d3d11_1.h>
#include <array>
#include <span>

using namespace trace_test;
namespace temporal = neuralfx::temporal;

namespace {
constexpr UINT render_width = 24;
constexpr UINT render_height = 20;
constexpr UINT camera_offset = 16;

struct Targets {
    std::array<ComPtr<ID3D11RenderTargetView>, 4> geometry;
    std::array<ComPtr<ID3D11RenderTargetView>, 2> lighting;
    ComPtr<ID3D11Texture2D> depth;
    ComPtr<ID3D11DepthStencilView> depth_view;
    ComPtr<ID3D11Buffer> camera;
};

ComPtr<ID3D11Texture2D> texture(ID3D11Device* device, DXGI_FORMAT format, UINT bind) {
    D3D11_TEXTURE2D_DESC description{render_width, render_height, 1, 1, format, {1, 0},
        D3D11_USAGE_DEFAULT, bind, 0, 0};
    ComPtr<ID3D11Texture2D> result;
    device->CreateTexture2D(&description, nullptr, &result);
    return result;
}

bool color_target(ID3D11Device* device, ComPtr<ID3D11RenderTargetView>& view) {
    auto color = texture(device, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET);
    return color && SUCCEEDED(device->CreateRenderTargetView(color.Get(), nullptr, &view));
}

std::array<float, 32> camera_values() {
    std::array<float, 32> values{};
    for (size_t index = 0; index < values.size(); ++index) values[index] = static_cast<float>(index) + 0.5f;
    return values;
}

bool create_targets(ID3D11Device* device, Targets& targets) {
    for (auto& view : targets.geometry) if (!color_target(device, view)) return false;
    for (auto& view : targets.lighting) if (!color_target(device, view)) return false;
    targets.depth = texture(device, DXGI_FORMAT_R32G8X24_TYPELESS, D3D11_BIND_DEPTH_STENCIL);
    D3D11_DEPTH_STENCIL_VIEW_DESC view{DXGI_FORMAT_D32_FLOAT_S8X24_UINT, D3D11_DSV_DIMENSION_TEXTURE2D, 0, {}};
    if (!targets.depth || FAILED(device->CreateDepthStencilView(targets.depth.Get(), &view, &targets.depth_view)))
        return false;
    auto values = camera_values();
    std::array<float, 128> padded{};
    std::copy(values.begin(), values.end(), padded.begin() + camera_offset * 4);
    D3D11_BUFFER_DESC description{sizeof(padded), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA data{padded.data(), 0, 0};
    return SUCCEEDED(device->CreateBuffer(&description, &data, &targets.camera));
}

void bind_and_draw(ID3D11DeviceContext* context, std::span<const ComPtr<ID3D11RenderTargetView>> views,
    ID3D11DepthStencilView* depth) {
    std::array<ID3D11RenderTargetView*, 4> raw{};
    for (size_t index = 0; index < views.size(); ++index) raw[index] = views[index].Get();
    context->OMSetRenderTargets(static_cast<UINT>(views.size()), raw.data(), depth);
    context->DrawIndexed(3, 0, 0);
    context->DrawIndexed(3, 0, 0);
}

void game_frame(Device& gpu, const Targets& targets) {
    ComPtr<ID3D11DeviceContext1> context;
    gpu.context.As(&context);
    ID3D11Buffer* camera = targets.camera.Get();
    UINT first = camera_offset;
    UINT count = 16;
    context->VSSetConstantBuffers1(0, 1, &camera, &first, &count);
    bind_and_draw(gpu.context.Get(), targets.geometry, targets.depth_view.Get());
    bind_and_draw(gpu.context.Get(), targets.lighting, targets.depth_view.Get());
}

int check(const temporal::SceneSample& sample, const Targets& targets) {
    if (!sample.frame.render || sample.frame.render->width != render_width) return 20;
    if (sample.depth.Get() != targets.depth.Get()) return 21;
    if (!sample.camera) return 22;
    auto values = camera_values();
    for (size_t index = 0; index < 16; ++index) {
        if (sample.camera->view[index] != values[index]) return 23;
        if (sample.camera->view_projection[index] != values[16 + index]) return 24;
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
    if (!create_pipeline(gpu.device.Get(), pipeline) || !create_targets(gpu.device.Get(), targets)) return 2;
    bind_pipeline(gpu.context.Get(), pipeline);
    neuralfx::hook_context(gpu.swapchain.Get());
    if (!temporal::install_scene_hooks(gpu.context.Get())) return 3;
    ComPtr<ID3D11Texture2D> backbuffer;
    gpu.swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    auto& tracker = temporal::scene_tracker();
    tracker.start_frame(backbuffer.Get());
    game_frame(gpu, targets);
    if (tracker.finish_frame(gpu.context.Get()).frame.render) return 4;
    tracker.set_active(true);
    tracker.start_frame(backbuffer.Get());
    game_frame(gpu, targets);
    int result = check(tracker.finish_frame(gpu.context.Get()), targets);
    game_frame(gpu, targets);
    if (result == 0 && tracker.finish_frame(gpu.context.Get()).camera) result = 30;
    DestroyWindow(window);
    return result;
}
