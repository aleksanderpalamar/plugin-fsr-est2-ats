#include "diagnostics/motion_probe.hpp"
#include "diagnostics/motion_probe_format.hpp"
#include "diagnostics/motion_probe_targets.hpp"
#include "diagnostics/motion_tally.hpp"
#include "diagnostics/trace_trigger.hpp"
#include "renderer/camera_motion_pass.hpp"
#include "renderer/camera_reprojection_pass.hpp"
#include "renderer/context_state.hpp"
#include "renderer/motion_overlay_pass.hpp"
#include "temporal/scene_tracker.hpp"
#include "common/logger.hpp"

#include <d3d11.h>
#include <chrono>
#include <cmath>
#include <wrl/client.h>

namespace neuralfx::diagnostics {
namespace {
using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;

constexpr std::uint64_t logged_frames = 240;
constexpr float overlay_scale = 0.05f;

struct Probe {
    bool enabled = false;
    bool ready = false;
    std::uint64_t frame = 0;
    temporal::TemporalHistory history;
    MotionTally tally;
    CameraReprojectionPass reprojection;
    CameraMotionPass motion;
    MotionOverlayPass overlay;
    MotionProbeTargets targets;
};

struct Swapchain {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Texture2D> backbuffer;
};

Probe& probe() noexcept {
    static Probe instance;
    return instance;
}

std::optional<Swapchain> swapchain_of(IDXGISwapChain* swapchain) noexcept {
    Swapchain result;
    if (!swapchain || FAILED(swapchain->GetDevice(IID_PPV_ARGS(&result.device)))) return std::nullopt;
    result.device->GetImmediateContext(&result.context);
    if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&result.backbuffer))) || !result.context) return std::nullopt;
    return result;
}

bool initialize(Probe& state, ID3D11Device* device) noexcept {
    auto shaders = module_directory() / L"NeuralFX" / L"shaders";
    return SUCCEEDED(state.reprojection.initialize(device, shaders)) &&
        SUCCEEDED(state.motion.initialize(device, shaders)) && SUCCEEDED(state.overlay.initialize(device, shaders));
}

void toggle(Probe& state, const Swapchain& target) noexcept {
    state.enabled = !state.enabled;
    state.history.reset();
    state.tally = {};
    state.frame = 0;
    if (state.enabled && !temporal::install_scene_hooks(target.context.Get())) {
        log("Motion probe draw hooks unavailable");
        state.enabled = false;
    }
    if (state.enabled && !state.ready) state.ready = initialize(state, target.device.Get());
    if (state.enabled && !state.ready) log("Motion probe shaders unavailable");
    temporal::scene_tracker().set_active(state.enabled);
    log(state.enabled ? "Motion probe enabled" : "Motion probe disabled");
}

std::optional<temporal::JitterObservation> observed_jitter(const temporal::SceneSample& sample) {
    if (!sample.observation || !sample.frame.render) return std::nullopt;
    auto camera = temporal::camera_state(sample.observation->matrices, *sample.frame.render);
    if (!camera) return std::nullopt;
    return temporal::JitterObservation{sample.observation->frame, camera->jitter};
}

std::optional<float> camera_distance(const temporal::SceneSample& sample) {
    if (!sample.observation) return std::nullopt;
    const auto& view = sample.observation->matrices.view;
    return std::sqrt(view[3] * view[3] + view[7] * view[7] + view[11] * view[11]);
}

void draw_overlay(Probe& state, const Swapchain& target, const temporal::SceneSample& sample,
    temporal::RenderSize size, bool history_valid) noexcept {
    if (!state.targets.prepare(target.device.Get(), sample.depth.Get(), size)) return;
    D3D11_TEXTURE2D_DESC description{};
    target.backbuffer->GetDesc(&description);
    ComPtr<ID3D11RenderTargetView> output;
    if (FAILED(target.device->CreateRenderTargetView(target.backbuffer.Get(), nullptr, &output))) return;
    ContextState saved(target.context.Get());
    auto* camera = state.reprojection.run(target.context.Get(), sample.camera.Get(), size, history_valid);
    if (!camera) return;
    state.targets.copy_depth(target.context.Get(), sample.depth.Get());
    state.motion.render(target.context.Get(), {state.targets.depth(), camera, state.targets.motion_target()}, size);
    state.overlay.render(target.context.Get(),
        {state.targets.motion(), output.Get(), description.Width / 2, description.Height / 2}, overlay_scale);
}

void record(Probe& state, const temporal::TemporalDecision& decision, const temporal::SceneSample& sample,
    std::int64_t cpu_us) {
    if (state.frame < logged_frames) log(motion_line(sample.index, decision, sample.frame.render, cpu_us));
    ++state.frame;
    state.tally.add(decision, cpu_us, camera_distance(sample));
    if (state.tally.frames < logged_frames) return;
    log(tally_line(state.tally));
    state.tally = {};
}

void inspect(Probe& state, const Swapchain& target) {
    auto start = Clock::now();
    auto sample = temporal::scene_tracker().finish_frame(target.context.Get());
    auto decision = state.history.advance({sample.index, sample.frame.render, sample.camera != nullptr,
        sample.frame.temporal_resolve, observed_jitter(sample)});
    auto cpu_us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
    record(state, decision, sample, cpu_us);
    if (!state.ready || !decision.jitter || !sample.camera || !sample.depth) return;
    draw_overlay(state, target, sample, *sample.frame.render, decision.status == temporal::TemporalStatus::Ready);
}
}

void present_motion_probe(IDXGISwapChain* swapchain) noexcept {
    try {
        auto& state = probe();
        if (!state.enabled) return;
        if (auto target = swapchain_of(swapchain)) inspect(state, *target);
    } catch (...) {
        log("Motion probe frame failed");
    }
}

void motion_probe_frame_end(IDXGISwapChain* swapchain) noexcept {
    try {
        static TraceTrigger trigger(module_directory() / L"NeuralFX" / L"motion-debug");
        auto result = trigger.poll(Clock::now());
        if (result == TriggerResult::Failed) log("Motion probe request could not be removed");
        auto& state = probe();
        if (result != TriggerResult::Requested && !state.enabled) return;
        auto target = swapchain_of(swapchain);
        if (!target) return;
        if (result == TriggerResult::Requested) toggle(state, *target);
        if (state.enabled) temporal::scene_tracker().start_frame(target->backbuffer.Get());
    } catch (...) {
        log("Motion probe request failed");
    }
}
}
