#include "common/logger.hpp"
#include "dxgi/context_hook.hpp"
#include "upscale_stage_helpers.hpp"

#include <thread>

using namespace stage_test;

namespace {
constexpr int frames_to_run = 400;

ResolveValues driving_resolve() {
    auto values = scene_resolve();
    resolve_test::move_by(values, 0.0f, 0.0f, 0.25f);
    return values;
}

bool burst_exists(const std::filesystem::path& directory) {
    return std::filesystem::exists(directory / "fsr-1-1-color.bmp") && std::filesystem::exists(directory / "fsr-1-3-resolve.txt");
}
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW, 0, 0, 32, 32, nullptr, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    Device gpu;
    Pipeline pipeline;
    Targets targets;
    if (!window || FAILED(create_device(window, gpu))) return 1;
    CameraValues camera{};
    if (!create_pipeline(gpu.device.Get(), pipeline) || !create_targets(gpu.device.Get(), camera, targets, driving_resolve()))
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
    auto captures = neuralfx::module_directory() / L"NeuralFX" / L"captures";
    std::filesystem::remove_all(captures);
    std::filesystem::remove(neuralfx::module_directory() / L"NeuralFX" / L"fsr-capture");
    auto& tracker = temporal::scene_tracker();
    tracker.attach(&stage);
    tracker.set_active(true);
    for (int frame = 0; frame < frames_to_run; ++frame) {
        clear_inputs(gpu, targets, back_view.Get());
        stage.begin_frame();
        tracker.start_frame(backbuffer.Get());
        game_frame(gpu, targets, true);
        gpu.context->Flush();
        stage.observe(tracker.finish_frame(gpu.context.Get()));
        if (frame < upscale::motion_trigger_sustained_frames - 5 && std::filesystem::exists(captures)) return 5;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    DestroyWindow(window);
    return burst_exists(captures) ? 0 : 6;
}
