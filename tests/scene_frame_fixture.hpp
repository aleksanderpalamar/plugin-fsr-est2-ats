#pragma once

#include "frame_trace_hook_fixture.hpp"

#include <d3d11_1.h>
#include <array>
#include <span>

namespace scene_test {
using namespace trace_test;

inline constexpr UINT render_width = 24;
inline constexpr UINT render_height = 20;
inline constexpr UINT camera_offset = 16;
inline constexpr UINT resolve_offset = 16;

using CameraValues = std::array<float, 32>;
using ResolveValues = std::array<float, 64>;

inline ResolveValues resolve_pattern() {
    ResolveValues values{};
    for (size_t index = 0; index < values.size(); ++index) values[index] = static_cast<float>(index) + 0.25f;
    return values;
}

struct Targets {
    std::array<ComPtr<ID3D11RenderTargetView>, 4> geometry;
    std::array<ComPtr<ID3D11RenderTargetView>, 2> lighting;
    ComPtr<ID3D11Texture2D> depth;
    ComPtr<ID3D11DepthStencilView> depth_view;
    ComPtr<ID3D11Buffer> camera;
    ComPtr<ID3D11Buffer> resolve;
    ComPtr<ID3D11RenderTargetView> output;
    ComPtr<ID3D11Texture2D> current_color;
    ComPtr<ID3D11RenderTargetView> current_color_target;
    ComPtr<ID3D11ShaderResourceView> current_color_view;
    ComPtr<ID3D11Texture2D> tonemapped;
    ComPtr<ID3D11ShaderResourceView> tonemapped_view;
};

inline ComPtr<ID3D11Texture2D> scene_texture(ID3D11Device* device, DXGI_FORMAT format, UINT bind) {
    D3D11_TEXTURE2D_DESC description{render_width, render_height, 1, 1, format, {1, 0},
        D3D11_USAGE_DEFAULT, bind, 0, 0};
    ComPtr<ID3D11Texture2D> result;
    device->CreateTexture2D(&description, nullptr, &result);
    return result;
}

inline bool color_target(ID3D11Device* device, ComPtr<ID3D11RenderTargetView>& view) {
    auto color = scene_texture(device, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET);
    return color && SUCCEEDED(device->CreateRenderTargetView(color.Get(), nullptr, &view));
}

inline bool create_camera(ID3D11Device* device, const CameraValues& values, ComPtr<ID3D11Buffer>& camera) {
    std::array<float, 128> padded{};
    std::copy(values.begin(), values.end(), padded.begin() + camera_offset * 4);
    D3D11_BUFFER_DESC description{sizeof(padded), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA data{padded.data(), 0, 0};
    return SUCCEEDED(device->CreateBuffer(&description, &data, &camera));
}

inline bool create_resolve(ID3D11Device* device, const ResolveValues& values, ComPtr<ID3D11Buffer>& resolve) {
    std::array<float, 128> padded{};
    std::copy(values.begin(), values.end(), padded.begin() + resolve_offset * 4);
    D3D11_BUFFER_DESC description{sizeof(padded), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA data{padded.data(), 0, 0};
    return SUCCEEDED(device->CreateBuffer(&description, &data, &resolve));
}

inline bool create_post_inputs(ID3D11Device* device, Targets& targets) {
    targets.current_color = scene_texture(device, DXGI_FORMAT_R16G16B16A16_FLOAT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    targets.tonemapped = scene_texture(device, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D11_BIND_SHADER_RESOURCE);
    if (!targets.current_color || !targets.tonemapped) return false;
    return SUCCEEDED(device->CreateRenderTargetView(targets.current_color.Get(), nullptr, &targets.current_color_target)) &&
        SUCCEEDED(device->CreateShaderResourceView(targets.current_color.Get(), nullptr, &targets.current_color_view)) &&
        SUCCEEDED(device->CreateShaderResourceView(targets.tonemapped.Get(), nullptr, &targets.tonemapped_view));
}

inline bool create_targets(ID3D11Device* device, const CameraValues& values, Targets& targets,
    const ResolveValues& resolve = resolve_pattern()) {
    for (auto& view : targets.geometry) if (!color_target(device, view)) return false;
    for (auto& view : targets.lighting) if (!color_target(device, view)) return false;
    targets.depth = scene_texture(device, DXGI_FORMAT_R32G8X24_TYPELESS,
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
    D3D11_DEPTH_STENCIL_VIEW_DESC view{DXGI_FORMAT_D32_FLOAT_S8X24_UINT, D3D11_DSV_DIMENSION_TEXTURE2D, 0, {}};
    if (!targets.depth || FAILED(device->CreateDepthStencilView(targets.depth.Get(), &view, &targets.depth_view)))
        return false;
    return create_camera(device, values, targets.camera) && create_resolve(device, resolve, targets.resolve) &&
        create_post_inputs(device, targets);
}

inline void bind_and_draw(ID3D11DeviceContext* context, std::span<const ComPtr<ID3D11RenderTargetView>> views,
    ID3D11DepthStencilView* depth) {
    std::array<ID3D11RenderTargetView*, 4> raw{};
    for (size_t index = 0; index < views.size(); ++index) raw[index] = views[index].Get();
    context->OMSetRenderTargets(static_cast<UINT>(views.size()), raw.data(), depth);
    context->DrawIndexed(3, 0, 0);
    context->DrawIndexed(3, 0, 0);
}

inline void game_frame(Device& gpu, const Targets& targets, bool temporal_resolve) {
    ComPtr<ID3D11DeviceContext1> context;
    gpu.context.As(&context);
    ID3D11Buffer* camera = targets.camera.Get();
    UINT first = camera_offset;
    UINT count = 16;
    context->VSSetConstantBuffers1(0, 1, &camera, &first, &count);
    bind_and_draw(gpu.context.Get(), targets.geometry, targets.depth_view.Get());
    bind_and_draw(gpu.context.Get(), targets.lighting, targets.depth_view.Get());
    if (!temporal_resolve) return;
    ID3D11Buffer* resolve = targets.resolve.Get();
    UINT resolve_first = resolve_offset;
    UINT resolve_count = 16;
    context->PSSetConstantBuffers1(0, 1, &resolve, &resolve_first, &resolve_count);
    ID3D11ShaderResourceView* current = targets.current_color_view.Get();
    context->PSSetShaderResources(0, 1, &current);
    bind_and_draw(gpu.context.Get(), targets.lighting, nullptr);
    if (!targets.output) return;
    ID3D11RenderTargetView* output = targets.output.Get();
    ID3D11ShaderResourceView* tonemapped = targets.tonemapped_view.Get();
    gpu.context->PSSetShaderResources(0, 1, &tonemapped);
    gpu.context->OMSetRenderTargets(1, &output, nullptr);
    gpu.context->DrawIndexed(3, 0, 0);
    gpu.context->DrawIndexed(3, 0, 0);
}
}
