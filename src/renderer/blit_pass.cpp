#include "renderer/blit_pass.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
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
    draw_fullscreen(context, {vertex_.Get(), pixel_.Get(), {source, nullptr, nullptr}, target, nullptr, nullptr,
        size.width, size.height});
}

void BlitPass::reset() noexcept {
    pixel_.Reset();
    vertex_.Reset();
}
}
