#include "renderer/compute_state.hpp"

namespace neuralfx {
namespace {
template <typename T>
void attach(std::array<Microsoft::WRL::ComPtr<T>, ComputeState::slots>& owned,
    const std::array<T*, ComputeState::slots>& raw) {
    for (UINT index = 0; index < ComputeState::slots; ++index) owned[index].Attach(raw[index]);
}

template <typename T>
std::array<T*, ComputeState::slots> raw_of(const std::array<Microsoft::WRL::ComPtr<T>, ComputeState::slots>& owned) {
    std::array<T*, ComputeState::slots> raw{};
    for (UINT index = 0; index < ComputeState::slots; ++index) raw[index] = owned[index].Get();
    return raw;
}
}

ComputeState::ComputeState(ID3D11DeviceContext* context) noexcept : context_(context) {
    if (!context_) return;
    context_->CSGetShader(shader_.GetAddressOf(), nullptr, nullptr);
    std::array<ID3D11Buffer*, slots> constants{};
    std::array<ID3D11ShaderResourceView*, slots> resources{};
    std::array<ID3D11UnorderedAccessView*, slots> outputs{};
    context_->CSGetConstantBuffers(0, slots, constants.data());
    context_->CSGetShaderResources(0, slots, resources.data());
    context_->CSGetUnorderedAccessViews(0, slots, outputs.data());
    attach(constants_, constants);
    attach(resources_, resources);
    attach(outputs_, outputs);
}

ComputeState::~ComputeState() {
    if (!context_) return;
    auto constants = raw_of(constants_);
    auto resources = raw_of(resources_);
    auto outputs = raw_of(outputs_);
    std::array<UINT, slots> keep_counters;
    keep_counters.fill(static_cast<UINT>(-1));
    context_->CSSetUnorderedAccessViews(0, slots, outputs.data(), keep_counters.data());
    context_->CSSetShaderResources(0, slots, resources.data());
    context_->CSSetConstantBuffers(0, slots, constants.data());
    context_->CSSetShader(shader_.Get(), nullptr, 0);
}
}
