#include "renderer/blit_pass.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
namespace {
using Microsoft::WRL::ComPtr;

DXGI_FORMAT raw_format(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) return DXGI_FORMAT_B8G8R8A8_UNORM;
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) return DXGI_FORMAT_R8G8B8A8_UNORM;
    return format;
}

ComPtr<ID3D11RenderTargetView> raw_target(ID3D11RenderTargetView* target) {
    D3D11_RENDER_TARGET_VIEW_DESC description{};
    target->GetDesc(&description);
    DXGI_FORMAT raw = raw_format(description.Format);
    if (raw == description.Format) return target;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11Resource> resource;
    target->GetDevice(&device);
    target->GetResource(&resource);
    description.Format = raw;
    ComPtr<ID3D11RenderTargetView> view;
    if (FAILED(device->CreateRenderTargetView(resource.Get(), &description, &view))) return target;
    return view;
}
}

HRESULT BlitPass::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_vertex_shader(device, shader_directory / L"fullscreen.hlsl", "VSMain", vertex_);
    if (FAILED(result)) return result;
    return create_pixel_shader(device, shader_directory / L"fullscreen.hlsl", "CopyMain", pixel_);
}

void BlitPass::render(ID3D11DeviceContext* context, ID3D11ShaderResourceView* source, ID3D11RenderTargetView* target,
    temporal::RenderSize size) noexcept {
    if (!context || !pixel_ || !source || !target) return;
    auto raw = raw_target(target);
    draw_fullscreen(context, {vertex_.Get(), pixel_.Get(), {source, nullptr, nullptr}, raw.Get(), nullptr, nullptr,
        size.width, size.height});
}

void BlitPass::reset() noexcept {
    pixel_.Reset();
    vertex_.Reset();
}
}
