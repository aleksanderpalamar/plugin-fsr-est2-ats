#include "temporal/bound_constants.hpp"

#include <d3d11_1.h>

namespace neuralfx::temporal {
namespace {
using Microsoft::WRL::ComPtr;

constexpr UINT constant_bytes = 16;

void read_range(ID3D11DeviceContext1* context, ConstantStage stage, UINT slot, ID3D11Buffer** buffer, UINT* first,
    UINT* count) {
    if (stage == ConstantStage::Vertex)
        context->VSGetConstantBuffers1(slot, 1, buffer, first, count);
    else
        context->PSGetConstantBuffers1(slot, 1, buffer, first, count);
}

void read_whole(ID3D11DeviceContext* context, ConstantStage stage, UINT slot, ID3D11Buffer** buffer) {
    if (stage == ConstantStage::Vertex)
        context->VSGetConstantBuffers(slot, 1, buffer);
    else
        context->PSGetConstantBuffers(slot, 1, buffer);
}
}

BoundConstants bound_constants(ID3D11DeviceContext* context, ConstantStage stage, UINT slot) noexcept {
    BoundConstants bound;
    ID3D11Buffer* buffer = nullptr;
    UINT first = 0;
    UINT count = 0;
    ComPtr<ID3D11DeviceContext1> extended;
    if (SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(&extended))))
        read_range(extended.Get(), stage, slot, &buffer, &first, &count);
    else
        read_whole(context, stage, slot, &buffer);
    bound.buffer.Attach(buffer);
    bound.offset_bytes = first * constant_bytes;
    bound.range_bytes = count * constant_bytes;
    return bound;
}

bool holds(const BoundConstants& bound, UINT bytes) noexcept {
    if (!bound.buffer) return false;
    D3D11_BUFFER_DESC description{};
    bound.buffer->GetDesc(&description);
    bool range_fits = bound.range_bytes == 0 || bound.range_bytes >= bytes;
    return range_fits && bound.offset_bytes + bytes <= description.ByteWidth;
}
}
