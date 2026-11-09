#include "diagnostics/frame_tracer.hpp"
#include "dxgi/context_hook.hpp"
#include "frame_trace_hook_fixture.hpp"

using namespace trace_test;

namespace {
struct Scene {
    ComPtr<ID3D11Texture2D> color;
    ComPtr<ID3D11Texture2D> history;
    ComPtr<ID3D11Texture2D> previous;
    ComPtr<ID3D11Texture2D> depth;
    ComPtr<ID3D11Texture2D> backbuffer;
    ComPtr<ID3D11RenderTargetView> color_target;
    ComPtr<ID3D11RenderTargetView> output;
    ComPtr<ID3D11DepthStencilView> depth_target;
    std::vector<ComPtr<ID3D11ShaderResourceView>> inputs;
};

bool create_scene(Device& gpu, Scene& scene) {
    auto* device = gpu.device.Get();
    constexpr UINT target = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    scene.color = create_texture(device, DXGI_FORMAT_R16G16_FLOAT, target);
    scene.history = create_texture(device, DXGI_FORMAT_R16G16_FLOAT, D3D11_BIND_SHADER_RESOURCE);
    scene.previous = create_texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, target);
    scene.depth = create_texture(device, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
    if (!scene.color || !scene.history || !scene.previous || !scene.depth) return false;
    if (FAILED(gpu.swapchain->GetBuffer(0, IID_PPV_ARGS(&scene.backbuffer)))) return false;
    if (FAILED(device->CreateRenderTargetView(scene.color.Get(), nullptr, &scene.color_target))) return false;
    if (FAILED(device->CreateRenderTargetView(scene.backbuffer.Get(), nullptr, &scene.output))) return false;
    if (FAILED(device->CreateDepthStencilView(scene.depth.Get(), nullptr, &scene.depth_target))) return false;
    for (auto* texture : {scene.color.Get(), scene.history.Get(), scene.previous.Get()}) {
        ComPtr<ID3D11ShaderResourceView> view;
        if (FAILED(device->CreateShaderResourceView(texture, nullptr, &view))) return false;
        scene.inputs.push_back(view);
    }
    return true;
}

void game_frame(Device& gpu, const Pipeline& pipeline, Scene& scene) {
    const float black[4]{};
    auto* context = gpu.context.Get();
    context->ClearRenderTargetView(scene.color_target.Get(), black);
    context->ClearDepthStencilView(scene.depth_target.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    auto* color_target = scene.color_target.Get();
    context->OMSetRenderTargets(1, &color_target, scene.depth_target.Get());
    D3D11_VIEWPORT viewport{0.0f, 0.0f, 16.0f, 16.0f, 0.0f, 1.0f};
    context->RSSetViewports(1, &viewport);
    context->Draw(3, 0);
    context->DrawIndexed(3, 0, 0);
    context->CopyResource(scene.history.Get(), scene.color.Get());
    auto* output = scene.output.Get();
    context->OMSetRenderTargets(1, &output, nullptr);
    context->PSSetShader(pipeline.pixel.Get(), nullptr, 0);
    ID3D11ShaderResourceView* inputs[3]{scene.inputs[0].Get(), scene.inputs[1].Get(), scene.inputs[2].Get()};
    context->PSSetShaderResources(0, 3, inputs);
    context->DrawInstanced(3, 1, 0, 0);
}

std::vector<std::string> expected(const Scene& scene, const Pipeline& pipeline) {
    auto color = hex(scene.color.Get());
    auto previous = hex(scene.previous.Get());
    auto depth = hex(scene.depth.Get());
    auto history = hex(scene.history.Get());
    return {
        "Trace frame begin backbuffer=bb 32x32 fmt=28 samples=1",
        "Trace 0 clear-color rt=" + color,
        "Trace 1 clear-depth ds=" + depth,
        "Trace 2 pass rt=" + color + " ds=" + depth + " vp=16x16 draws=2",
        "Trace 3 constants vs slot=0 buffer=" + hex(pipeline.constants.Get()) + " offset=0 size=16 values=1,2,3,4",
        "Trace 4 copy src=" + color + " dst=" + history,
        "Trace 5 pass rt=bb draws=1 reads=" + color + "," + previous,
        "Trace 6 draw count=3 ps=" + hex(pipeline.pixel.Get()) + " inputs=" + color + "," + previous,
        "Trace target bb 32x32 fmt=28 samples=1 color writes=1 reads=0",
        "Trace target " + color + " 32x32 fmt=34 samples=1 color writes=1 reads=2",
        "Trace target " + previous + " 32x32 fmt=28 samples=1 color writes=0 reads=1",
        "Trace target " + depth + " 32x32 fmt=45 samples=1 depth writes=1 reads=0",
        "Trace target " + history + " 32x32 fmt=34 samples=1 color writes=1 reads=0",
        "Trace frame end entries=7 draws=3 dropped=0"};
}

void sort_targets(std::vector<std::string>& lines) {
    auto first = std::find_if(lines.begin(), lines.end(), [](const std::string& line) {
        return line.starts_with("Trace target ") && !line.starts_with("Trace target bb");
    });
    auto last = std::find_if(first, lines.end(), [](const std::string& line) {
        return line.starts_with("Trace frame end");
    });
    std::sort(first, last);
}
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW,
        0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return 1;
    Device gpu;
    Pipeline pipeline;
    Scene scene;
    if (FAILED(create_device(window, gpu))) return 2;
    if (!create_pipeline(gpu.device.Get(), pipeline) || !create_scene(gpu, scene)) return 3;
    bind_pipeline(gpu.context.Get(), pipeline);
    auto* constants = pipeline.constants.Get();
    gpu.context->VSSetConstantBuffers(0, 1, &constants);
    write_constants(gpu.context.Get(), constants);
    neuralfx::hook_context(gpu.swapchain.Get());
    namespace trace = neuralfx::diagnostics;
    trace::trace_present_end(gpu.swapchain.Get());
    game_frame(gpu, pipeline, scene);
    if (!trace::trace_present_begin().empty()) return 4;
    if (!trace::request_capture(1)) return 5;
    trace::trace_present_end(gpu.swapchain.Get());
    game_frame(gpu, pipeline, scene);
    auto lines = trace::trace_present_begin();
    auto wanted = expected(scene, pipeline);
    sort_targets(lines);
    sort_targets(wanted);
    if (!same_lines(lines, wanted)) return 6;
    trace::trace_present_end(gpu.swapchain.Get());
    game_frame(gpu, pipeline, scene);
    if (!trace::trace_present_begin().empty()) return 7;
    DestroyWindow(window);
    return 0;
}
