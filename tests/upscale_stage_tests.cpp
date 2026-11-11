#include "common/logger.hpp"
#include "dxgi/context_hook.hpp"
#include "upscale_stage_helpers.hpp"

#ifdef NEURALFX_REAL_BACKEND
#include "upscale/dx12/dx12_backend.hpp"
#endif

#include <cmath>
#include <iostream>
#include <thread>

using namespace stage_test;

namespace {
constexpr temporal::RenderSize output_size{32, 32};
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW, 0, 0, 32, 32, nullptr, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    Device gpu;
    Pipeline pipeline;
    Targets targets;
    if (!window || FAILED(create_device(window, gpu))) return 1;
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
#ifdef NEURALFX_REAL_BACKEND
    upscale::Dx12Backend backend(gpu.device.Get(), gpu.context.Get(), neuralfx::module_directory());
    if (backend.providers().empty()) return 77;
    upscale::UpscaleStage stage(backend, upscale::UpscaleMode::Auto);
    if (FAILED(stage.initialize(gpu.device.Get(), neuralfx::module_directory() / L"NeuralFX" / L"shaders"))) return 4;
    auto& tracker = temporal::scene_tracker();
    tracker.attach(&stage);
    tracker.set_active(true);
    for (int frame = 0; frame < 80; ++frame) {
        clear_inputs(gpu, targets, back_view.Get());
        stage.begin_frame();
        tracker.start_frame(backbuffer.Get());
        game_frame(gpu, targets, true);
        gpu.context->Flush();
        stage.observe(tracker.finish_frame(gpu.context.Get()));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    neuralfx::log(upscale::counters_line(stage.counters()));
    const auto& counters = stage.counters();
    if (counters.upscaled == 0) return 20;
    if (counters.contract_invalid != 0 || counters.backend_invalid != 0 || counters.failed != 0) return 21;
#else
    RecordingBackend backend;
    backend.context = gpu.context.Get();
    backend.device = gpu.device.Get();
    upscale::UpscaleStage stage(backend, upscale::UpscaleMode::Auto);
    if (FAILED(stage.initialize(gpu.device.Get(), neuralfx::module_directory() / L"NeuralFX" / L"shaders"))) return 4;
    auto& tracker = temporal::scene_tracker();
    tracker.attach(&stage);
    tracker.set_active(true);
    bool upscaled = false;
    for (int frame = 0; frame < 60 && !upscaled; ++frame) {
        clear_inputs(gpu, targets, back_view.Get());
        stage.begin_frame();
        tracker.start_frame(backbuffer.Get());
        int before = backend.upscales;
        game_frame(gpu, targets, true);
        gpu.context->Flush();
        stage.observe(tracker.finish_frame(gpu.context.Get()));
        auto corner = read_pixel<std::array<std::uint8_t, 4>>(gpu, backbuffer.Get(), 4, 4);
        if (!corner) return 5;
        bool green = (*corner)[0] == 0 && (*corner)[1] == 255 && (*corner)[2] == 0;
        if (backend.upscales == before && (*corner)[0] != 255) return 6;
        if (backend.upscales > before) {
            if (!green) return 7;
            if (!holds_current_color(gpu, targets.lighting[0].Get()) || !holds_current_color(gpu, targets.lighting[1].Get()))
                return 9;
            upscaled = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if (!upscaled) return 8;
    const auto& contract = backend.last;
    if (contract.render.width != render_width || contract.render.height != render_height) return 10;
    if (contract.output.width != output_size.width || contract.output.height != output_size.height) return 11;
    if (!temporal::sequence_phase(contract.jitter)) return 12;
    if (std::fabs(contract.camera.vertical_fov - 0.84678f) > 5e-4f || contract.camera.near_plane != 0.2f ||
        contract.camera.far_plane != 1600.0f) return 13;
    if (!contract.reset) return 14;
    if (!backend.opened.depth_inverted || !backend.opened.color_non_linear) return 15;
#endif
    DestroyWindow(window);
    return 0;
}
