#include "dxgi/context_hook.hpp"
#include "scene_frame_fixture.hpp"
#include "temporal/scene_tracker.hpp"

using namespace scene_test;
namespace temporal = neuralfx::temporal;

namespace {
CameraValues camera_values() {
    CameraValues values{};
    for (size_t index = 0; index < values.size(); ++index) values[index] = static_cast<float>(index) + 0.5f;
    return values;
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
    return sample.frame.temporal_resolve ? 0 : 25;
}
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW, 0, 0, 32, 32,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    Device gpu;
    Pipeline pipeline;
    Targets targets;
    if (!window || FAILED(create_device(window, gpu))) return 1;
    if (!create_pipeline(gpu.device.Get(), pipeline) || !create_targets(gpu.device.Get(), camera_values(), targets))
        return 2;
    bind_pipeline(gpu.context.Get(), pipeline);
    neuralfx::hook_context(gpu.swapchain.Get());
    if (!temporal::install_scene_hooks(gpu.context.Get())) return 3;
    ComPtr<ID3D11Texture2D> backbuffer;
    gpu.swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    auto& tracker = temporal::scene_tracker();
    tracker.start_frame(backbuffer.Get());
    game_frame(gpu, targets, true);
    if (tracker.finish_frame(gpu.context.Get()).frame.render) return 4;
    tracker.set_active(true);
    tracker.start_frame(backbuffer.Get());
    game_frame(gpu, targets, true);
    int result = check(tracker.finish_frame(gpu.context.Get()), targets);
    game_frame(gpu, targets, true);
    if (result == 0 && tracker.finish_frame(gpu.context.Get()).camera) result = 30;
    DestroyWindow(window);
    return result;
}
