#include "diagnostics/motion_probe.hpp"
#include "diagnostics/constant_dump.hpp"
#include "diagnostics/motion_probe_format.hpp"
#include "renderer/motion_targets.hpp"
#include "diagnostics/probe_journal.hpp"
#include "diagnostics/trace_trigger.hpp"
#include "renderer/camera_motion_pass.hpp"
#include "renderer/context_state.hpp"
#include "renderer/motion_overlay_pass.hpp"
#include "renderer/resolve_frame_pass.hpp"
#include "temporal/observation_review.hpp"
#include "temporal/resolve_review.hpp"
#include "temporal/scene_tracker.hpp"
#include "common/logger.hpp"

#include <d3d11.h>
#include <chrono>
#include <wrl/client.h>

namespace neuralfx::diagnostics {
namespace {
using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;

constexpr std::uint32_t dumped_frames = 4;
constexpr float overlay_scale = 0.05f;

struct Probe {
    bool enabled = false;
    bool ready = false;
    temporal::ObservationReview review;
    temporal::TemporalHistory history;
    ProbeJournal journal;
    ConstantDump dump;
    ResolveFramePass resolve_frame;
    CameraMotionPass motion;
    MotionOverlayPass overlay;
    MotionTargets targets;
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
    return SUCCEEDED(state.resolve_frame.initialize(device, shaders)) &&
        SUCCEEDED(state.motion.initialize(device, shaders)) && SUCCEEDED(state.overlay.initialize(device, shaders));
}

void toggle(Probe& state, const Swapchain& target) noexcept {
    state.enabled = !state.enabled;
    state.history.reset();
    state.review.reset();
    state.journal.reset();
    if (state.enabled && !temporal::install_scene_hooks(target.context.Get())) {
        log("Motion probe draw hooks unavailable");
        state.enabled = false;
    }
    if (state.enabled && !state.ready) state.ready = initialize(state, target.device.Get());
    if (state.enabled && !state.ready) log("Motion probe shaders unavailable");
    if (state.enabled)
        temporal::scene_tracker().attach(&state.dump);
    else
        temporal::scene_tracker().detach(&state.dump);
    temporal::scene_tracker().set_active(state.enabled);
    log(state.enabled ? "Motion probe enabled" : "Motion probe disabled");
}

temporal::ObservationReport review(Probe& state, const temporal::SceneSample& sample) noexcept {
    if (!sample.frame.render) return {};
    return state.review.review(sample.observations, *sample.frame.render);
}

temporal::ResolveReport review_resolves(const temporal::SceneSample& sample) noexcept {
    if (!sample.frame.render) return {};
    return temporal::review_resolves(sample.resolves, *sample.frame.render);
}

void draw_overlay(Probe& state, const Swapchain& target, const temporal::SceneSample& sample,
    temporal::RenderSize size) noexcept {
    if (!state.targets.prepare(target.device.Get(), sample.depth.Get(), size)) return;
    D3D11_TEXTURE2D_DESC description{};
    target.backbuffer->GetDesc(&description);
    ComPtr<ID3D11RenderTargetView> output;
    if (FAILED(target.device->CreateRenderTargetView(target.backbuffer.Get(), nullptr, &output))) return;
    ContextState saved(target.context.Get());
    auto* camera = state.resolve_frame.run(target.context.Get(), sample.resolve.Get(), size);
    if (!camera) return;
    state.targets.copy_depth(target.context.Get(), sample.depth.Get());
    state.motion.render(target.context.Get(), {state.targets.depth(), camera, state.targets.motion_target()}, size);
    state.overlay.render(target.context.Get(),
        {state.targets.motion(), output.Get(), description.Width / 2, description.Height / 2}, overlay_scale);
}

void inspect(Probe& state, const Swapchain& target) {
    auto start = Clock::now();
    auto sample = temporal::scene_tracker().finish_frame(target.context.Get());
    auto report = review(state, sample);
    auto decision = state.history.advance({sample.index, sample.frame.render, sample.camera != nullptr,
        sample.frame.temporal_resolve, report.jitters, report.jump, !report.rejections.empty()});
    auto cpu_us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
    state.journal.record(decision, {sample.index, sample.frame.render, cpu_us, sample.times}, report,
        review_resolves(sample));
    for (const auto& line : state.dump.finish_frame()) log(line);
    if (!state.ready || !sample.resolve || !sample.depth || !sample.frame.render) return;
    draw_overlay(state, target, sample, *sample.frame.render);
}

void poll_constant_request(Probe& state) {
    static TraceTrigger trigger(module_directory() / L"NeuralFX" / L"constants-debug");
    auto result = trigger.poll(Clock::now());
    if (result == TriggerResult::Failed) log("Constant dump request could not be removed");
    if (result != TriggerResult::Requested) return;
    if (!state.enabled) {
        log("Constant dump needs the motion probe");
        return;
    }
    state.dump.arm(dumped_frames);
    log("Constant dump armed for " + std::to_string(dumped_frames) + " frames");
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
        if (result != TriggerResult::Requested && !state.enabled) return poll_constant_request(state);
        auto target = swapchain_of(swapchain);
        if (!target) return;
        if (result == TriggerResult::Requested) toggle(state, *target);
        poll_constant_request(state);
        if (state.enabled) temporal::scene_tracker().start_frame(target->backbuffer.Get());
    } catch (...) {
        log("Motion probe request failed");
    }
}
}
