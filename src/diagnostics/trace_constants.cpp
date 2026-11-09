#include "diagnostics/trace_constants.hpp"
#include "diagnostics/trace_resources.hpp"

#include <d3d11_1.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <wrl/client.h>

namespace neuralfx::diagnostics {
namespace {
using Microsoft::WRL::ComPtr;

constexpr std::uint32_t constant_bytes = 16;

struct Binding {
    std::array<ID3D11Buffer*, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> buffers{};
    std::array<UINT, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> first{};
    std::array<UINT, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> count{};
};

void bound_extended(ID3D11DeviceContext1* context, ShaderStage stage, UINT slots, Binding& binding) {
    if (stage == ShaderStage::Pixel)
        context->PSGetConstantBuffers1(0, slots, binding.buffers.data(), binding.first.data(), binding.count.data());
    else
        context->VSGetConstantBuffers1(0, slots, binding.buffers.data(), binding.first.data(), binding.count.data());
}

Binding bound_buffers(ID3D11DeviceContext* context, ShaderStage stage, UINT slots) {
    Binding binding;
    ComPtr<ID3D11DeviceContext1> extended;
    if (SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(&extended))))
        bound_extended(extended.Get(), stage, slots, binding);
    else if (stage == ShaderStage::Pixel)
        context->PSGetConstantBuffers(0, slots, binding.buffers.data());
    else
        context->VSGetConstantBuffers(0, slots, binding.buffers.data());
    return binding;
}

std::vector<float> read_range(ID3D11DeviceContext* context, ID3D11Buffer* buffer, UINT offset, UINT bytes) {
    ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    D3D11_BUFFER_DESC description{(bytes + constant_bytes - 1) / constant_bytes * constant_bytes,
        D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
    ComPtr<ID3D11Buffer> staging;
    if (!device || FAILED(device->CreateBuffer(&description, nullptr, &staging))) return {};
    D3D11_BOX box{offset, 0, 0, offset + bytes, 1, 1};
    context->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, buffer, 0, &box);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return {};
    std::vector<float> values(bytes / sizeof(float));
    std::memcpy(values.data(), mapped.pData, values.size() * sizeof(float));
    context->Unmap(staging.Get(), 0);
    return values;
}

std::optional<ConstantSnapshot> snapshot(ID3D11DeviceContext* context, ID3D11Buffer* buffer, std::uint32_t slot,
    UINT first, UINT count, std::uint32_t floats) {
    D3D11_BUFFER_DESC description{};
    buffer->GetDesc(&description);
    UINT offset = first * constant_bytes;
    if (offset >= description.ByteWidth) return std::nullopt;
    UINT available = description.ByteWidth - offset;
    UINT size = count > 0 ? std::min(count * constant_bytes, available) : available;
    UINT bytes = std::min<UINT>(size, floats * sizeof(float));
    return ConstantSnapshot{slot, resource_id(buffer), offset, size, read_range(context, buffer, offset, bytes)};
}
}

std::vector<ConstantSnapshot> read_constants(ID3D11DeviceContext* context, ShaderStage stage, std::uint32_t slots,
    std::uint32_t floats) noexcept {
    try {
        UINT count = std::min<UINT>(slots, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);
        auto binding = bound_buffers(context, stage, count);
        std::vector<ConstantSnapshot> snapshots;
        for (UINT slot = 0; slot < count; ++slot) {
            ComPtr<ID3D11Buffer> buffer;
            buffer.Attach(binding.buffers[slot]);
            if (!buffer) continue;
            auto taken = snapshot(context, buffer.Get(), slot, binding.first[slot], binding.count[slot], floats);
            if (!taken) continue;
            taken->stage = stage;
            snapshots.push_back(std::move(*taken));
        }
        return snapshots;
    } catch (...) {
        return {};
    }
}
}
