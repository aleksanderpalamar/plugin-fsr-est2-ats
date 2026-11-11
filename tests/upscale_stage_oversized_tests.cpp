#include "common/logger.hpp"
#include "dxgi/context_hook.hpp"
#include "upscale_stage_helpers.hpp"

#include <iostream>

using namespace stage_test;

namespace {
constexpr UINT smaller_than_render = 16;
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW, 0, 0, 32, 32, nullptr, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    Device gpu;
    Pipeline pipeline;
    Targets targets;
    if (!window || FAILED(create_device(window, gpu, smaller_than_render, smaller_than_render))) return 1;
    CameraValues camera{};
    if (!create_pipeline(gpu.device.Get(), pipeline) || !create_targets(gpu.device.Get(), camera, targets, scene_resolve()))
        return 2;
    bind_pipeline(gpu.context.Get(), pipeline);
    neuralfx::hook_context(gpu.swapchain.Get());
    if (!temporal::install_scene_hooks(gpu.context.Get())) return 3;
    ComPtr<ID3D11Texture2D> backbuffer;
    ComPtr<ID3D11RenderTargetView> back_view;
    gpu.swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    gpu.device->CreateRenderTargetView(backbuffer.Get(), nullptr, &back_view);
    targets.output = back_view;
    RecordingBackend backend;
    backend.context = gpu.context.Get();
    backend.device = gpu.device.Get();
    upscale::UpscaleStage stage(backend, upscale::UpscaleMode::Auto);
    if (FAILED(stage.initialize(gpu.device.Get(), neuralfx::module_directory() / L"NeuralFX" / L"shaders"))) return 4;
    auto& tracker = temporal::scene_tracker();
    tracker.attach(&stage);
    tracker.set_active(true);
    for (int frame = 0; frame < 40; ++frame) {
        clear_inputs(gpu, targets, back_view.Get());
        stage.begin_frame();
        tracker.start_frame(backbuffer.Get());
        game_frame(gpu, targets, true);
        gpu.context->Flush();
        stage.observe(tracker.finish_frame(gpu.context.Get()));
        if (holds_current_color(gpu, targets.lighting[0].Get())) return 5;
    }
    const auto& counters = stage.counters();
    if (backend.upscales != 0 || counters.upscaled != 0) return 6;
    if (counters.replaced != 0) return 7;
    if (counters.contract_invalid != 0) return 8;
    DestroyWindow(window);
    return 0;
}
