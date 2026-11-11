#include "renderer/camera_motion_pass.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
namespace {
constexpr float cabin_depth_threshold = 0.5f;

struct MotionParameters {
    float size[2];
    float cabin_depth;
    float padding;
};
static_assert(sizeof(MotionParameters) == 16);
}

HRESULT CameraMotionPass::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_vertex_shader(device, shader_directory / L"fullscreen.hlsl", "VSMain", vertex_);
    if (FAILED(result)) return result;
    result = create_pixel_shader(device, shader_directory / L"camera_motion.hlsl", "MotionMain", pixel_);
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = sizeof(MotionParameters);
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    return device->CreateBuffer(&description, nullptr, &constants_);
}

void CameraMotionPass::render(ID3D11DeviceContext* context, const CameraMotionTargets& targets,
    temporal::RenderSize size) noexcept {
    if (!context || !pixel_ || !constants_ || !targets.depth || !targets.camera || !targets.output) return;
    MotionParameters parameters{{static_cast<float>(size.width), static_cast<float>(size.height)}, cabin_depth_threshold, 0.0f};
    context->UpdateSubresource(constants_.Get(), 0, nullptr, &parameters, 0, 0);
    draw_fullscreen(context, {vertex_.Get(), pixel_.Get(), {targets.depth, targets.camera, nullptr},
        targets.output, constants_.Get(), nullptr, size.width, size.height});
}

void CameraMotionPass::reset() noexcept {
    constants_.Reset();
    pixel_.Reset();
    vertex_.Reset();
}
}
