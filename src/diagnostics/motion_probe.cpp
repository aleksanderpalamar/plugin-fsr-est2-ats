#include "diagnostics/motion_probe.hpp"
#include "diagnostics/motion_probe_format.hpp"
#include "diagnostics/motion_probe_targets.hpp"
#include "diagnostics/trace_trigger.hpp"
#include "renderer/camera_motion_pass.hpp"
#include "renderer/context_state.hpp"
#include "renderer/motion_overlay_pass.hpp"
#include "temporal/scene_tracker.hpp"
#include "common/logger.hpp"

#include <d3d11.h>
#include <chrono>
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
    return SUCCEEDED(state.motion.initialize(device, shaders)) && SUCCEEDED(state.overlay.initialize(device, shaders));
}

void toggle(Probe& state, const Swapchain& target) noexcept {
    state.enabled = !state.enabled;
    state.history.reset();
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

void draw_overlay(Probe& state, const Swapchain& target, const temporal::SceneSample& sample,
    const temporal::MotionConstants& motion) noexcept {
    if (!state.targets.prepare(target.device.Get(), sample.depth.Get(), motion.size)) return;
    D3D11_TEXTURE2D_DESC description{};
    target.backbuffer->GetDesc(&description);
    ComPtr<ID3D11RenderTargetView> output;
    if (FAILED(target.device->CreateRenderTargetView(target.backbuffer.Get(), nullptr, &output))) return;
    ContextState saved(target.context.Get());
    state.targets.copy_depth(target.context.Get(), sample.depth.Get());
    state.motion.render(target.context.Get(), {state.targets.depth(), state.targets.motion_target()}, motion);
    state.overlay.render(target.context.Get(),
        {state.targets.motion(), output.Get(), description.Width / 2, description.Height / 2}, overlay_scale);
}

void inspect(Probe& state, const Swapchain& target) {
    auto start = Clock::now();
    auto sample = temporal::scene_tracker().finish_frame(target.context.Get());
    auto wait = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
    auto decision = state.history.advance({sample.frame.render, sample.camera, sample.frame.temporal_resolve});
    if (state.frame < logged_frames) log(motion_line(state.frame, decision, sample.frame.render, wait));
    ++state.frame;
    if (state.ready && decision.motion && sample.depth) draw_overlay(state, target, sample, *decision.motion);
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
