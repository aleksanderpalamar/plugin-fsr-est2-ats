#include "dxgi/context_hook.hpp"
#include "scene_frame_fixture.hpp"
#include "temporal/scene_tracker.hpp"

#include <thread>

using namespace scene_test;
namespace temporal = neuralfx::temporal;

namespace {
CameraValues camera_values() {
    CameraValues values{};
    for (size_t index = 0; index < values.size(); ++index) values[index] = static_cast<float>(index) + 0.5f;
    return values;
}

bool holds_values(const temporal::CameraMatrices& matrices) {
    auto values = camera_values();
    for (size_t index = 0; index < 16; ++index) {
        if (matrices.view[index] != values[index]) return false;
        if (matrices.view_projection[index] != values[16 + index]) return false;
    }
    return true;
}

std::optional<temporal::CameraMatrices> read_camera(Device& gpu, ID3D11Buffer* camera) {
    D3D11_BUFFER_DESC description{128, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
    ComPtr<ID3D11Buffer> staging;
    if (FAILED(gpu.device->CreateBuffer(&description, nullptr, &staging))) return std::nullopt;
    gpu.context->CopyResource(staging.Get(), camera);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    temporal::CameraMatrices matrices;
    const auto* values = static_cast<const float*>(mapped.pData);
    std::copy(values, values + 16, matrices.view.begin());
    std::copy(values + 16, values + 32, matrices.view_projection.begin());
    gpu.context->Unmap(staging.Get(), 0);
    return matrices;
}

int check(Device& gpu, const temporal::SceneSample& sample, const Targets& targets) {
    if (!sample.frame.render || sample.frame.render->width != render_width) return 20;
    if (sample.depth.Get() != targets.depth.Get()) return 21;
    if (!sample.camera) return 22;
    auto copied = read_camera(gpu, sample.camera.Get());
    if (!copied || !holds_values(*copied)) return 23;
    return sample.frame.temporal_resolve ? 0 : 25;
}

int observe(Device& gpu, const Targets& targets, ID3D11Texture2D* backbuffer) {
    auto& tracker = temporal::scene_tracker();
    for (int attempt = 0; attempt < 50; ++attempt) {
        tracker.start_frame(backbuffer);
        game_frame(gpu, targets, true);
        gpu.context->Flush();
        auto sample = tracker.finish_frame(gpu.context.Get());
        if (!sample.observation) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        if (sample.observation->frame > sample.index) return 40;
        return holds_values(sample.observation->matrices) ? 0 : 41;
    }
    return 42;
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
    int result = check(gpu, tracker.finish_frame(gpu.context.Get()), targets);
    game_frame(gpu, targets, true);
    if (result == 0 && tracker.finish_frame(gpu.context.Get()).camera) result = 30;
    if (result == 0) result = observe(gpu, targets, backbuffer.Get());
    DestroyWindow(window);
    return result;
}
