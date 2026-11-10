#include "renderer/depth_to_float_pass.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
HRESULT DepthToFloatPass::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_vertex_shader(device, shader_directory / L"fullscreen.hlsl", "VSMain", vertex_);
    if (FAILED(result)) return result;
    return create_pixel_shader(device, shader_directory / L"depth_to_float.hlsl", "DepthMain", pixel_);
}

void DepthToFloatPass::render(ID3D11DeviceContext* context, const DepthToFloatTargets& targets,
    temporal::RenderSize size) noexcept {
    if (!context || !pixel_ || !targets.depth || !targets.output) return;
    draw_fullscreen(context, {vertex_.Get(), pixel_.Get(), {targets.depth, nullptr, nullptr}, targets.output, nullptr,
        nullptr, size.width, size.height});
}

void DepthToFloatPass::reset() noexcept {
    pixel_.Reset();
    vertex_.Reset();
}
}
