#include "renderer/resolve_frame_pass.hpp"
#include "renderer/compute_state.hpp"
#include "gpu/shader_compiler.hpp"

#include <array>
#include <cmath>

namespace neuralfx {
namespace {
constexpr float degrees_to_radians = 0.017453292519943295f;

struct ResolveParameters {
    float size[2];
    float max_move;
    float min_turn_cosine;
};
static_assert(sizeof(ResolveParameters) == 16);
}

HRESULT ResolveFramePass::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_compute_shader(device, shader_directory / L"resolve_frame.hlsl", "ResolveMain", shader_);
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{sizeof(ResolveParameters), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    result = device->CreateBuffer(&description, nullptr, &parameters_);
    return FAILED(result) ? result : slot_.create(device);
}

ID3D11ShaderResourceView* ResolveFramePass::run(ID3D11DeviceContext* context, ID3D11Buffer* game_resolve,
    temporal::RenderSize size, temporal::JumpLimits limits) noexcept {
    if (!context || !game_resolve || !shader_ || !slot_.output) return nullptr;
    ResolveParameters parameters{{static_cast<float>(size.width), static_cast<float>(size.height)},
        limits.distance_m, std::cos(limits.angle_deg * degrees_to_radians)};
    context->UpdateSubresource(parameters_.Get(), 0, nullptr, &parameters, 0, 0);
    ComputeState saved(context);
    std::array<ID3D11Buffer*, 2> constants{game_resolve, parameters_.Get()};
    std::array<ID3D11UnorderedAccessView*, 2> outputs{slot_.output.Get(), nullptr};
    context->CSSetShader(shader_.Get(), nullptr, 0);
    context->CSSetConstantBuffers(0, 2, constants.data());
    context->CSSetUnorderedAccessViews(0, 2, outputs.data(), nullptr);
    context->Dispatch(1, 1, 1);
    return slot_.view.Get();
}

void ResolveFramePass::reset() noexcept {
    slot_ = {};
    parameters_.Reset();
    shader_.Reset();
}
}
