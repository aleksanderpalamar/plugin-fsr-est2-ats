#include "renderer/motion_overlay_pass.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
namespace {
struct OverlayParameters {
    float size[2];
    float scale;
    float padding;
};
static_assert(sizeof(OverlayParameters) == 16);
}

HRESULT MotionOverlayPass::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_vertex_shader(device, shader_directory / L"fullscreen.hlsl", "VSMain", vertex_);
    if (FAILED(result)) return result;
    result = create_pixel_shader(device, shader_directory / L"motion_debug.hlsl", "OverlayMain", pixel_);
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = sizeof(OverlayParameters);
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    return device->CreateBuffer(&description, nullptr, &constants_);
}

void MotionOverlayPass::render(ID3D11DeviceContext* context, const MotionOverlayTargets& targets, float scale) noexcept {
    if (!context || !pixel_ || !constants_ || !targets.motion || !targets.output) return;
    OverlayParameters parameters{{static_cast<float>(targets.width), static_cast<float>(targets.height)}, scale, 0.0f};
    context->UpdateSubresource(constants_.Get(), 0, nullptr, &parameters, 0, 0);
    draw_fullscreen(context, {vertex_.Get(), pixel_.Get(), {targets.motion, nullptr, nullptr},
        targets.output, constants_.Get(), nullptr, targets.width, targets.height});
}

void MotionOverlayPass::reset() noexcept {
    constants_.Reset();
    pixel_.Reset();
    vertex_.Reset();
}
}
