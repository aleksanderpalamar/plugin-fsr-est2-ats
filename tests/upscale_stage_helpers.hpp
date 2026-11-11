#pragma once

#include "resolve_values.hpp"
#include "scene_frame_fixture.hpp"
#include "temporal/jitter_sequence.hpp"
#include "upscale/upscale_stage.hpp"

#include <cmath>

namespace stage_test {
using namespace scene_test;
namespace temporal = neuralfx::temporal;
namespace upscale = neuralfx::upscale;

struct RecordingBackend final : upscale::UpscalingBackend {
    ID3D11DeviceContext* context = nullptr;
    ID3D11Device* device = nullptr;
    int upscales = 0;
    bool reactive_seen = false;
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
        reactive_seen = resources.reactive != nullptr;
        last = contract;
        ComPtr<ID3D11RenderTargetView> view;
        if (FAILED(device->CreateRenderTargetView(resources.output, nullptr, &view))) return upscale::UpscaleStatus::BackendFailure;
        const float green[4]{0.0f, 1.0f, 0.0f, 1.0f};
        context->ClearRenderTargetView(view.Get(), green);
        return upscale::UpscaleStatus::Ready;
    }
    void close() override { ++closes; }
};

inline float half_to_float(std::uint16_t value) {
    int exponent = (value >> 10) & 31;
    int mantissa = value & 1023;
    float sign = (value >> 15) ? -1.0f : 1.0f;
    if (exponent == 0) return sign * std::ldexp(static_cast<float>(mantissa), -24);
    return sign * std::ldexp(static_cast<float>(mantissa + 1024), exponent - 25);
}

template <typename Pixel>
inline std::optional<Pixel> read_pixel(Device& gpu, ID3D11Texture2D* texture, UINT x, UINT y) {
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

inline ResolveValues scene_resolve() {
    auto values = resolve_test::game_values(static_cast<float>(render_width), static_cast<float>(render_height));
    auto jitter = temporal::sequence_jitter(5);
    resolve_test::jitter_by(values, jitter.x, jitter.y);
    values[5] = 2.2190392f;
    return values;
}

inline void clear_inputs(Device& gpu, const Targets& targets, ID3D11RenderTargetView* backbuffer) {
    const float white[4]{1.0f, 1.0f, 1.0f, 1.0f};
    const float color[4]{0.5f, 0.25f, 0.125f, 1.0f};
    gpu.context->ClearRenderTargetView(backbuffer, white);
    gpu.context->ClearRenderTargetView(targets.current_color_target.Get(), color);
    gpu.context->ClearDepthStencilView(targets.depth_view.Get(), D3D11_CLEAR_DEPTH, 0.5f, 0);
}

inline ComPtr<ID3D11Texture2D> texture_of(ID3D11RenderTargetView* view) {
    ComPtr<ID3D11Resource> resource;
    ComPtr<ID3D11Texture2D> texture;
    view->GetResource(&resource);
    resource.As(&texture);
    return texture;
}

inline bool holds_current_color(Device& gpu, ID3D11RenderTargetView* resolve_target) {
    auto texels = read_pixel<std::array<std::uint16_t, 4>>(gpu, texture_of(resolve_target).Get(), 0, 0);
    if (!texels) return false;
    const float wanted[4]{0.5f, 0.25f, 0.125f, 1.0f};
    for (int channel = 0; channel < 4; ++channel)
        if (std::fabs(half_to_float((*texels)[channel]) - wanted[channel]) > 1e-3f) return false;
    return true;
}
}
