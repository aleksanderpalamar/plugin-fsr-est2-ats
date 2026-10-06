#include "renderer/renderer.hpp"
#include "renderer/context_state.hpp"
#include "renderer/photoreal_constants.hpp"
#include "renderer/screen_space_constants.hpp"
#include "renderer/capture_policy.hpp"
#include "dxgi/depth_capture.hpp"
#include "common/logger.hpp"

#include <windows.h>
#include <chrono>
#include <string>
#include <system_error>

namespace neuralfx {
Renderer& Renderer::instance() noexcept {
    static Renderer renderer;
    return renderer;
}

void Renderer::poll_hotkey() noexcept {
    bool down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
    if (down && !key_down_) config_.enabled = !config_.enabled;
    key_down_ = down;
}

void Renderer::load_lut() {
    lut_.reset();
    if (config_.lut_path.empty()) return;
    auto path = module_directory() / L"NeuralFX" / config_.lut_path;
    if (FAILED(lut_.load(device_.Get(), path))) log("LUT unavailable: " + path.string());
}

void Renderer::poll_config() {
    if (config_.mode != TestMode::Photoreal && config_.mode != TestMode::Raytracing) return;
    auto now = std::chrono::steady_clock::now();
    if (now < next_config_poll_) return;
    next_config_poll_ = now + std::chrono::seconds(1);
    std::error_code error;
    auto modified = std::filesystem::last_write_time(
        module_directory() / L"NeuralFX" / L"neuralfx.ini", error);
    if (error || modified == config_write_) return;
    config_write_ = modified;
    auto updated = load_config();
    config_.photoreal = updated.photoreal;
    config_.sharpness = updated.sharpness;
    config_.lut_path = updated.lut_path;
    load_lut();
}

void Renderer::present(IDXGISwapChain* swapchain) {
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock() || !swapchain) return;
    if (active_swapchain_ != swapchain) {
        reset_unlocked();
        active_swapchain_ = swapchain;
    }
    if (!initialized_ && !failed_) ensure_initialized(swapchain);
    if (initialized_) poll_config();
    poll_hotkey();
    DepthCapture::instance().set_active(captures_depth(initialized_, config_));
    if (!initialized_ || !config_.enabled || config_.mode == TestMode::Hook) return;
    profiler_.poll(context_.Get());
    profiler_.begin(context_.Get());
    ContextState previous(context_.Get());
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> current_depth;
    if (config_.mode == TestMode::Raytracing)
        context_->OMGetRenderTargets(0, nullptr, &current_depth);
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    if (current_depth) DepthCapture::instance().capture(context_.Get(), current_depth.Get());
    context_->CopyResource(resources_.copy(), resources_.backbuffer());
    if (config_.mode == TestMode::Copy) {
        context_->CopyResource(resources_.backbuffer(), resources_.copy());
        profiler_.end(context_.Get());
        return;
    }
    if (config_.mode == TestMode::Photoreal || config_.mode == TestMode::Raytracing) {
        ID3D11ShaderResourceView* input = resources_.input();
        auto depth = config_.mode == TestMode::Raytracing
            ? DepthCapture::instance().take_view() : nullptr;
        if (depth) {
            render_screen_space(depth.Get());
            input = resources_.traced_input();
        }
        render_photoreal(input);
    } else {
        PhotorealConstants parameters{};
        parameters.rcas_strength = config_.sharpness;
        context_->UpdateSubresource(constants_.Get(), 0, nullptr, &parameters, 0, 0);
        draw(config_.mode == TestMode::Rcas ? rcas_pixel_.Get() : copy_pixel_.Get(),
            resources_.input(), resources_.output());
    }
    profiler_.end(context_.Get());
}

void Renderer::reset(IDXGISwapChain* swapchain) noexcept {
    std::lock_guard lock(mutex_);
    if (!resets_active_capture(active_swapchain_, swapchain)) return;
    reset_unlocked();
}

void Renderer::reset_unlocked() noexcept {
    resources_.reset();
    profiler_.reset();
    constants_.Reset();
    sampler_.Reset();
    neural_residual_.Reset();
    lut_.reset();
    finish_pixel_.Reset();
    trace_pixel_.Reset();
    photoreal_pixel_.Reset();
    rcas_pixel_.Reset();
    copy_pixel_.Reset();
    vertex_.Reset();
    trace_constants_.Reset();
    DepthCapture::instance().reset();
    context_.Reset();
    device_.Reset();
    initialized_ = false;
    failed_ = false;
    active_swapchain_ = nullptr;
    frame_index_ = 0.0f;
    config_write_ = {};
    next_config_poll_ = {};
}

void Renderer::set_neural_residual(ID3D11ShaderResourceView* resource) noexcept {
    std::lock_guard lock(mutex_);
    if (!resource) {
        neural_residual_.Reset();
        return;
    }
    Microsoft::WRL::ComPtr<ID3D11Device> owner;
    resource->GetDevice(&owner);
    if (owner.Get() != device_.Get()) return;
    neural_residual_ = resource;
}

void Renderer::ensure_initialized(IDXGISwapChain* swapchain) {
    config_ = load_config();
    std::error_code error;
    config_write_ = std::filesystem::last_write_time(
        module_directory() / L"NeuralFX" / L"neuralfx.ini", error);
    HRESULT result = initialize(swapchain);
    failed_ = FAILED(result);
    initialized_ = !failed_;
    if (initialized_ && config_.mode == TestMode::Raytracing)
        DepthCapture::instance().configure(device_.Get(), resources_.width(), resources_.height());
    if (failed_) log("Renderer initialization failed: " + std::to_string(static_cast<unsigned long>(result)));
}

HRESULT Renderer::initialize(IDXGISwapChain* swapchain) {
    HRESULT result = swapchain->GetDevice(IID_PPV_ARGS(&device_));
    if (FAILED(result)) return result;
    device_->GetImmediateContext(&context_);
    if (!context_) return E_FAIL;
    bool photoreal = config_.mode == TestMode::Photoreal || config_.mode == TestMode::Raytracing;
    result = resources_.initialize(device_.Get(), swapchain, photoreal,
        config_.mode == TestMode::Raytracing);
    if (FAILED(result)) return result;
    result = compile_shaders();
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = sizeof(PhotorealConstants);
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    result = device_->CreateBuffer(&description, nullptr, &constants_);
    if (FAILED(result)) return result;
    if (config_.mode == TestMode::Raytracing) {
        description.ByteWidth = sizeof(ScreenSpaceConstants);
        result = device_->CreateBuffer(&description, nullptr, &trace_constants_);
        if (FAILED(result)) return result;
    }
    result = photoreal ? initialize_photoreal() : S_OK;
    if (FAILED(result)) return result;
    if (FAILED(profiler_.initialize(device_.Get()))) log("GPU timing unavailable");
    log("D3D11 renderer ready: " + std::to_string(resources_.width()) + "x" + std::to_string(resources_.height()));
    log("UI isolation unavailable; post-processing affects HUD");
    return S_OK;
}

HRESULT Renderer::initialize_photoreal() {
    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    HRESULT result = device_->CreateSamplerState(&sampler, &sampler_);
    if (FAILED(result)) return result;
    load_lut();
    return S_OK;
}
}
