#include "renderer/camera_motion_pass.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
namespace {
struct MotionParameters {
    temporal::Matrix4 reprojection;
    float jitter[2];
    float size[2];
};
static_assert(sizeof(MotionParameters) % 16 == 0);

MotionParameters parameters_of(const temporal::MotionConstants& constants) {
    MotionParameters parameters{};
    parameters.reprojection = constants.reprojection;
    parameters.jitter[0] = constants.jitter.x;
    parameters.jitter[1] = constants.jitter.y;
    parameters.size[0] = static_cast<float>(constants.size.width);
    parameters.size[1] = static_cast<float>(constants.size.height);
    return parameters;
}
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
    const temporal::MotionConstants& constants) noexcept {
    if (!context || !pixel_ || !constants_ || !targets.depth || !targets.output) return;
    MotionParameters parameters = parameters_of(constants);
    context->UpdateSubresource(constants_.Get(), 0, nullptr, &parameters, 0, 0);
    draw_fullscreen(context, {vertex_.Get(), pixel_.Get(), {targets.depth, nullptr, nullptr},
        targets.output, constants_.Get(), nullptr, constants.size.width, constants.size.height});
}

void CameraMotionPass::reset() noexcept {
    constants_.Reset();
    pixel_.Reset();
    vertex_.Reset();
}
}
