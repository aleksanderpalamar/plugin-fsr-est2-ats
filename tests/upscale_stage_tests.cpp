#include "common/logger.hpp"
#include "dxgi/context_hook.hpp"
#include "resolve_values.hpp"
#include "scene_frame_fixture.hpp"
#include "temporal/jitter_sequence.hpp"
#include "upscale/upscale_stage.hpp"

#include <cmath>
#include <iostream>
#include <thread>

using namespace scene_test;
namespace temporal = neuralfx::temporal;
namespace upscale = neuralfx::upscale;

namespace {
constexpr temporal::RenderSize output_size{32, 32};

struct RecordingBackend final : upscale::UpscalingBackend {
    ID3D11DeviceContext* context = nullptr;
    ID3D11Device* device = nullptr;
    int upscales = 0;
    int closes = 0;
    upscale::FrameContract last;
    upscale::OpenParameters opened;

    std::vector<upscale::ProviderVersion> providers() override { return {{7, "3.1.6"}}; }
    upscale::UpscaleStatus open(const upscale::ProviderVersion&, const upscale::OpenParameters& parameters) override {
        opened = parameters;
        return upscale::UpscaleStatus::Ready;
    }
    std::string provider_name() const override { return "3.1.6"; }
    upscale::UpscaleStatus upscale(const upscale::FrameContract& contract, const upscale::FrameResources& resources) override {
        ++upscales;
        last = contract;
        ComPtr<ID3D11RenderTargetView> view;
        if (FAILED(device->CreateRenderTargetView(resources.output, nullptr, &view))) return upscale::UpscaleStatus::BackendFailure;
        const float green[4]{0.0f, 1.0f, 0.0f, 1.0f};
        context->ClearRenderTargetView(view.Get(), green);
        return upscale::UpscaleStatus::Ready;
    }
    void close() override { ++closes; }
};

float half_to_float(std::uint16_t value) {
    int exponent = (value >> 10) & 31;
    int mantissa = value & 1023;
    float sign = (value >> 15) ? -1.0f : 1.0f;
    if (exponent == 0) return sign * std::ldexp(static_cast<float>(mantissa), -24);
    return sign * std::ldexp(static_cast<float>(mantissa + 1024), exponent - 25);
}

template <typename Pixel>
std::optional<Pixel> read_pixel(Device& gpu, ID3D11Texture2D* texture, UINT x, UINT y) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    description.BindFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(gpu.device->CreateTexture2D(&description, nullptr, &staging))) return std::nullopt;
    gpu.context->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    Pixel pixel{};
    std::memcpy(&pixel, static_cast<const unsigned char*>(mapped.pData) + y * mapped.RowPitch + x * sizeof(Pixel), sizeof(Pixel));
    gpu.context->Unmap(staging.Get(), 0);
    return pixel;
}

ResolveValues scene_resolve() {
    auto values = resolve_test::game_values(static_cast<float>(render_width), static_cast<float>(render_height));
    auto jitter = temporal::sequence_jitter(5);
    resolve_test::jitter_by(values, jitter.x, jitter.y);
    values[5] = 2.2190392f;
    return values;
}

void clear_inputs(Device& gpu, const Targets& targets, ID3D11RenderTargetView* backbuffer) {
    const float white[4]{1.0f, 1.0f, 1.0f, 1.0f};
    const float color[4]{0.5f, 0.25f, 0.125f, 1.0f};
    gpu.context->ClearRenderTargetView(backbuffer, white);
    gpu.context->ClearRenderTargetView(targets.current_color_target.Get(), color);
    gpu.context->ClearDepthStencilView(targets.depth_view.Get(), D3D11_CLEAR_DEPTH, 0.5f, 0);
}

ComPtr<ID3D11Texture2D> texture_of(ID3D11RenderTargetView* view) {
    ComPtr<ID3D11Resource> resource;
    ComPtr<ID3D11Texture2D> texture;
    view->GetResource(&resource);
    resource.As(&texture);
    return texture;
}

bool holds_current_color(Device& gpu, ID3D11RenderTargetView* resolve_target) {
    auto texels = read_pixel<std::array<std::uint16_t, 4>>(gpu, texture_of(resolve_target).Get(), 0, 0);
    if (!texels) return false;
    const float wanted[4]{0.5f, 0.25f, 0.125f, 1.0f};
    for (int channel = 0; channel < 4; ++channel)
        if (std::fabs(half_to_float((*texels)[channel]) - wanted[channel]) > 1e-3f) return false;
    return true;
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
    DestroyWindow(window);
    return 0;
}
