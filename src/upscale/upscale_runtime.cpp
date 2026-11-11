#include "upscale/upscale_runtime.hpp"
#include "common/logger.hpp"
#include "config/config.hpp"
#include "dxgi/swapchain_access.hpp"
#include "temporal/scene_tracker.hpp"
#include "upscale/upscale_stage.hpp"

#ifdef NEURALFX_HAS_FSR_DX12
#include "upscale/dx12/dx12_backend.hpp"
#endif

#include <memory>

namespace neuralfx::upscale {
namespace {
struct Runtime {
    bool tried = false;
    bool active = false;
    std::unique_ptr<UpscalingBackend> backend;
    std::unique_ptr<UpscaleStage> stage;
};

Runtime& runtime() noexcept {
    static Runtime instance;
    return instance;
}

std::unique_ptr<UpscalingBackend> make_backend(const SwapchainAccess& access) {
#ifdef NEURALFX_HAS_FSR_DX12
    return std::make_unique<Dx12Backend>(access.device.Get(), access.context.Get(), module_directory());
#else
    (void)access;
    log("Upscale: this build has no FSR backend");
    return nullptr;
#endif
}

void start(Runtime& state, const SwapchainAccess& access) {
    state.tried = true;
    auto mode = load_config().upscale_mode;
    if (mode == UpscaleMode::Off) return;
    state.backend = make_backend(access);
    if (!state.backend) return;
    state.stage = std::make_unique<UpscaleStage>(*state.backend, mode);
    if (FAILED(state.stage->initialize(access.device.Get(), module_directory() / L"NeuralFX" / L"shaders"))) {
        log("Upscale: the shaders could not be created");
        state.stage.reset();
        return;
    }
    if (!temporal::install_scene_hooks(access.context.Get())) {
        log("Upscale: the draw hooks are unavailable");
        state.stage.reset();
        return;
    }
    temporal::scene_tracker().attach(state.stage.get());
    temporal::scene_tracker().set_active(true);
    state.active = true;
    log(mode == UpscaleMode::Strict ? "Upscale: enabled in strict mode" : "Upscale: enabled in auto mode");
}
}

bool upscale_active() noexcept {
    return runtime().active;
}

void upscale_frame_end(IDXGISwapChain* swapchain) noexcept {
    try {
        auto& state = runtime();
        if (state.tried && !state.active) return;
        auto access = access_swapchain(swapchain);
        if (!access) return;
        if (!state.tried) start(state, *access);
        if (!state.active) return;
        state.stage->begin_frame();
        temporal::scene_tracker().start_frame(access->backbuffer.Get());
    } catch (...) {
        log("Upscale: frame start failed");
    }
}

void upscale_present(IDXGISwapChain* swapchain) noexcept {
    try {
        auto& state = runtime();
        if (!state.active) return;
        auto access = access_swapchain(swapchain);
        if (!access) return;
        state.stage->observe(temporal::scene_tracker().finish_frame(access->context.Get()));
    } catch (...) {
        log("Upscale: frame end failed");
    }
}
}
