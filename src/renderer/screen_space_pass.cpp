#include "renderer/screen_space_pass.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
HRESULT ScreenSpacePass::initialize(ID3D11Device* device,
    const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_vertex_shader(device, shader_directory / L"fullscreen.hlsl",
        "VSMain", vertex_);
    if (FAILED(result)) return result;
    result = create_pixel_shader(device, shader_directory / L"raytracing.hlsl", "TraceMain", pixel_);
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = sizeof(ScreenSpaceConstants);
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    return device->CreateBuffer(&description, nullptr, &constants_);
}

void ScreenSpacePass::render(ID3D11DeviceContext* context, const ScreenSpaceTargets& targets,
    const ScreenSpaceConstants& constants) noexcept {
    if (!context || !pixel_ || !constants_) return;
    context->UpdateSubresource(constants_.Get(), 0, nullptr, &constants, 0, 0);
    draw_fullscreen(context, {vertex_.Get(), pixel_.Get(), {targets.color, targets.depth, nullptr},
        targets.output, constants_.Get(), nullptr, targets.width, targets.height});
}

void ScreenSpacePass::reset() noexcept {
    constants_.Reset();
    pixel_.Reset();
    vertex_.Reset();
}
}
