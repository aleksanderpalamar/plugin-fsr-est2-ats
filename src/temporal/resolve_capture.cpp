#include "temporal/resolve_capture.hpp"
#include "temporal/bound_constants.hpp"
#include "temporal/resolve_constants.hpp"

namespace neuralfx::temporal {
namespace {
constexpr UINT resolve_bytes = resolve_floats * sizeof(float);
}

ResolveCapture::ResolveCapture() noexcept : ring_(resolve_bytes) {}

bool ResolveCapture::capture(ID3D11DeviceContext* context, std::uint64_t frame) noexcept {
    auto bound = bound_constants(context, ConstantStage::Pixel, 0);
    if (!holds(bound, resolve_bytes) || !prepare(context)) return false;
    D3D11_BOX box{bound.offset_bytes, 0, 0, bound.offset_bytes + resolve_bytes, 1, 1};
    context->CopySubresourceRegion(resolve_.Get(), 0, 0, 0, 0, bound.buffer.Get(), 0, &box);
    return ring_.copy(context, frame, resolve_.Get(), 0);
}

bool ResolveCapture::prepare(ID3D11DeviceContext* context) noexcept {
    if (resolve_) return true;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    D3D11_BUFFER_DESC description{resolve_bytes, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    return device && SUCCEEDED(device->CreateBuffer(&description, nullptr, &resolve_));
}
}
