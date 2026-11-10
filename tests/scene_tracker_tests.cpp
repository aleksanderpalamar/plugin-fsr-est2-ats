#include "dxgi/context_hook.hpp"
#include "scene_frame_fixture.hpp"
#include "temporal/scene_tracker.hpp"

#include <iostream>
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

bool holds_resolve(const std::vector<float>& values) {
    auto expected = resolve_pattern();
    return values.size() == expected.size() && std::equal(values.begin(), values.end(), expected.begin());
}

int observe_resolve(Device& gpu, const Targets& targets, ID3D11Texture2D* backbuffer) {
    auto& tracker = temporal::scene_tracker();
    for (int attempt = 0; attempt < 50; ++attempt) {
        tracker.start_frame(backbuffer);
        game_frame(gpu, targets, true);
        gpu.context->Flush();
        auto sample = tracker.finish_frame(gpu.context.Get());
        if (sample.resolves.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        for (size_t index = 1; index < sample.resolves.size(); ++index)
            if (sample.resolves[index].frame <= sample.resolves[index - 1].frame) return 53;
        if (sample.resolves.back().frame > sample.index) return 50;
        return holds_resolve(sample.resolves.back().values) ? 0 : 51;
    }
    return 52;
}

std::optional<std::vector<float>> read_resolve(Device& gpu, ID3D11Buffer* resolve) {
    D3D11_BUFFER_DESC description{256, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
    ComPtr<ID3D11Buffer> staging;
    if (FAILED(gpu.device->CreateBuffer(&description, nullptr, &staging))) return std::nullopt;
    gpu.context->CopyResource(staging.Get(), resolve);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    const auto* values = static_cast<const float*>(mapped.pData);
    std::vector<float> copied(values, values + 64);
    gpu.context->Unmap(staging.Get(), 0);
    return copied;
}

int check(Device& gpu, const temporal::SceneSample& sample, const Targets& targets) {
    if (!sample.frame.render || sample.frame.render->width != render_width) return 20;
    if (sample.depth.Get() != targets.depth.Get()) return 21;
    if (!sample.camera) return 22;
    auto copied = read_camera(gpu, sample.camera.Get());
    if (!copied || !holds_values(*copied)) return 23;
    if (!sample.frame.temporal_resolve) return 25;
    if (!sample.resolve) return 26;
    auto resolve = read_resolve(gpu, sample.resolve.Get());
    return resolve && holds_resolve(*resolve) ? 0 : 27;
}

int observe(Device& gpu, const Targets& targets, ID3D11Texture2D* backbuffer) {
    auto& tracker = temporal::scene_tracker();
    for (int attempt = 0; attempt < 50; ++attempt) {
        tracker.start_frame(backbuffer);
        game_frame(gpu, targets, true);
        gpu.context->Flush();
        auto sample = tracker.finish_frame(gpu.context.Get());
        if (sample.observations.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        for (size_t index = 1; index < sample.observations.size(); ++index)
            if (sample.observations[index].frame <= sample.observations[index - 1].frame) return 43;
        if (sample.observations.back().frame > sample.index) return 40;
        return holds_values(sample.observations.back().matrices) ? 0 : 41;
    }
    return 42;
}
}

struct RecordingListener final : temporal::PassListener {
    temporal::DrawDecision output_decision = temporal::DrawDecision::Proceed;
    std::vector<temporal::PassEvent> events;

    bool draws_inside = false;

    temporal::DrawDecision first_draw(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept override {
        events.push_back(event);
        if (draws_inside) {
            ID3D11RenderTargetView* none = nullptr;
            context->OMSetRenderTargets(1, &none, nullptr);
            context->DrawIndexed(3, 0, 0);
        }
        return event.role == temporal::PassRole::Output ? output_decision : temporal::DrawDecision::Proceed;
    }
};

std::optional<UINT64> primitives(Device& gpu, const Targets& targets, ID3D11Texture2D* backbuffer) {
    D3D11_QUERY_DESC description{D3D11_QUERY_PIPELINE_STATISTICS, 0};
    ComPtr<ID3D11Query> query;
    if (FAILED(gpu.device->CreateQuery(&description, &query))) return std::nullopt;
    auto& tracker = temporal::scene_tracker();
    tracker.start_frame(backbuffer);
    gpu.context->Begin(query.Get());
    game_frame(gpu, targets, true);
    gpu.context->End(query.Get());
    tracker.finish_frame(gpu.context.Get());
    D3D11_QUERY_DATA_PIPELINE_STATISTICS data{};
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (gpu.context->GetData(query.Get(), &data, sizeof(data), 0) == S_OK) return data.IAPrimitives;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return std::nullopt;
}

int listener_events(Device& gpu, const Targets& targets, ID3D11Texture2D* backbuffer, RecordingListener& listener) {
    auto& tracker = temporal::scene_tracker();
    listener.events.clear();
    tracker.start_frame(backbuffer);
    game_frame(gpu, targets, true);
    tracker.finish_frame(gpu.context.Get());
    if (listener.events.size() != 3) return 60;
    using temporal::PassRole;
    if (listener.events[0].role != PassRole::SceneLighting || listener.events[1].role != PassRole::TemporalResolve)
        return 61;
    const auto& output = listener.events[2];
    if (output.role != PassRole::Output || !output.render || output.render->width != render_width) return 62;
    if (output.depth.Get() != targets.depth.Get() || !output.resolve) return 63;
    return listener.events[0].resolve ? 64 : 0;
}

int listener_veto(Device& gpu, const Targets& targets, ID3D11Texture2D* backbuffer) {
    auto& tracker = temporal::scene_tracker();
    RecordingListener listener;
    listener.draws_inside = true;
    tracker.attach(&listener);
    int result = listener_events(gpu, targets, backbuffer, listener);
    listener.draws_inside = false;
    auto proceeded = result == 0 ? primitives(gpu, targets, backbuffer) : std::nullopt;
    listener.output_decision = temporal::DrawDecision::Skip;
    auto skipped = result == 0 ? primitives(gpu, targets, backbuffer) : std::nullopt;
    tracker.detach(&listener);
    if (result != 0) return result;
    if (!proceeded || !skipped) {
        std::cerr << "pipeline statistics unavailable: the skipped draw was checked only through the decision\n";
        return 0;
    }
    if (*proceeded == *skipped + 1) return 0;
    std::cerr << "primitives proceed=" << *proceeded << " skip=" << *skipped << '\n';
    return 65;
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
    gpu.device->CreateRenderTargetView(backbuffer.Get(), nullptr, &targets.output);
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
    if (result == 0) result = observe_resolve(gpu, targets, backbuffer.Get());
    if (result == 0) result = listener_veto(gpu, targets, backbuffer.Get());
    DestroyWindow(window);
    return result;
}
