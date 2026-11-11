#include "renderer/reactive_mask_pass.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
namespace {
constexpr float tolerance_low = 0.06f;
constexpr float tolerance_high = 0.25f;

struct ReactiveParameters {
    float size[2];
    float low;
    float high;
};
static_assert(sizeof(ReactiveParameters) == 16);
}

HRESULT ReactiveMaskPass::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_vertex_shader(device, shader_directory / L"fullscreen.hlsl", "VSMain", vertex_);
    if (FAILED(result)) return result;
    result = create_pixel_shader(device, shader_directory / L"reactive_mask.hlsl", "ReactiveMain", pixel_);
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = sizeof(ReactiveParameters);
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    return device->CreateBuffer(&description, nullptr, &constants_);
}

void ReactiveMaskPass::render(ID3D11DeviceContext* context, const ReactiveMaskTargets& targets,
    temporal::RenderSize size) noexcept {
    if (!context || !pixel_ || !constants_ || !targets.current || !targets.previous || !targets.motion || !targets.output)
        return;
    ReactiveParameters parameters{{static_cast<float>(size.width), static_cast<float>(size.height)}, tolerance_low,
        tolerance_high};
    context->UpdateSubresource(constants_.Get(), 0, nullptr, &parameters, 0, 0);
    draw_fullscreen(context, {vertex_.Get(), pixel_.Get(), {targets.current, targets.previous, targets.motion},
        targets.output, constants_.Get(), nullptr, size.width, size.height});
}

void ReactiveMaskPass::reset() noexcept {
    constants_.Reset();
    pixel_.Reset();
    vertex_.Reset();
}
}
