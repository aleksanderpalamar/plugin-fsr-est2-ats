#include "renderer/camera_reprojection_pass.hpp"
#include "renderer/compute_state.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx {
namespace {
struct ReprojectionParameters {
    float size[2];
    std::uint32_t history_valid;
    std::uint32_t padding;
};
static_assert(sizeof(ReprojectionParameters) == 16);
}

HRESULT CameraReprojectionPass::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_compute_shader(device, shader_directory / L"camera_reprojection.hlsl", "ReprojectMain",
        shader_);
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{sizeof(ReprojectionParameters), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER,
        0, 0, 0};
    result = device->CreateBuffer(&description, nullptr, &parameters_);
    for (auto& slot : slots_) {
        if (FAILED(result)) return result;
        result = create_slot(device, slot);
    }
    return result;
}

HRESULT CameraReprojectionPass::create_slot(ID3D11Device* device, Slot& slot) {
    D3D11_BUFFER_DESC description{sizeof(GpuCameraFrame), D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        sizeof(GpuCameraFrame)};
    GpuCameraFrame empty{};
    D3D11_SUBRESOURCE_DATA data{&empty, 0, 0};
    HRESULT result = device->CreateBuffer(&description, &data, &slot.buffer);
    if (FAILED(result)) return result;
    result = device->CreateShaderResourceView(slot.buffer.Get(), nullptr, &slot.view);
    if (FAILED(result)) return result;
    return device->CreateUnorderedAccessView(slot.buffer.Get(), nullptr, &slot.output);
}

ID3D11ShaderResourceView* CameraReprojectionPass::run(ID3D11DeviceContext* context, ID3D11Buffer* game_camera,
    temporal::RenderSize size, bool history_valid) noexcept {
    if (!context || !game_camera || !shader_ || !slots_[0].output || !slots_[1].output) return nullptr;
    ReprojectionParameters parameters{{static_cast<float>(size.width), static_cast<float>(size.height)},
        history_valid ? 1u : 0u, 0u};
    context->UpdateSubresource(parameters_.Get(), 0, nullptr, &parameters, 0, 0);
    std::size_t next = 1 - current_;
    {
        ComputeState saved(context);
        std::array<ID3D11Buffer*, 2> constants{game_camera, parameters_.Get()};
        std::array<ID3D11ShaderResourceView*, 2> resources{slots_[current_].view.Get(), nullptr};
        std::array<ID3D11UnorderedAccessView*, 2> outputs{slots_[next].output.Get(), nullptr};
        context->CSSetShader(shader_.Get(), nullptr, 0);
        context->CSSetConstantBuffers(0, 2, constants.data());
        context->CSSetShaderResources(0, 2, resources.data());
        context->CSSetUnorderedAccessViews(0, 2, outputs.data(), nullptr);
        context->Dispatch(1, 1, 1);
    }
    current_ = next;
    return slots_[current_].view.Get();
}

void CameraReprojectionPass::reset() noexcept {
    for (auto& slot : slots_) slot = {};
    parameters_.Reset();
    shader_.Reset();
    current_ = 0;
}
}
