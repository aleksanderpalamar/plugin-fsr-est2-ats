#include "temporal/camera_readback.hpp"

#include <d3d11_1.h>
#include <algorithm>

namespace neuralfx::temporal {
namespace {
using Microsoft::WRL::ComPtr;

constexpr UINT constant_bytes = 16;
constexpr UINT camera_floats = 32;
constexpr UINT camera_bytes = camera_floats * sizeof(float);

struct BoundCamera {
    ComPtr<ID3D11Buffer> buffer;
    UINT first = 0;
    UINT count = 0;
};

BoundCamera bound_camera(ID3D11DeviceContext* context) {
    BoundCamera bound;
    ID3D11Buffer* buffer = nullptr;
    ComPtr<ID3D11DeviceContext1> extended;
    if (SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(&extended))))
        extended->VSGetConstantBuffers1(0, 1, &buffer, &bound.first, &bound.count);
    else
        context->VSGetConstantBuffers(0, 1, &buffer);
    bound.buffer.Attach(buffer);
    return bound;
}

bool holds_camera(const BoundCamera& bound) {
    D3D11_BUFFER_DESC description{};
    bound.buffer->GetDesc(&description);
    UINT offset = bound.first * constant_bytes;
    bool range_fits = bound.count == 0 || bound.count * constant_bytes >= camera_bytes;
    return range_fits && offset + camera_bytes <= description.ByteWidth;
}

ComPtr<ID3D11Buffer> create_staging(ID3D11DeviceContext* context) {
    ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    D3D11_BUFFER_DESC description{camera_bytes, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
    ComPtr<ID3D11Buffer> staging;
    if (device) device->CreateBuffer(&description, nullptr, &staging);
    return staging;
}
}

bool CameraReadback::request(ID3D11DeviceContext* context) noexcept {
    pending_ = false;
    auto bound = bound_camera(context);
    if (!bound.buffer || !holds_camera(bound)) return false;
    if (!staging_) staging_ = create_staging(context);
    if (!staging_) return false;
    UINT offset = bound.first * constant_bytes;
    D3D11_BOX box{offset, 0, 0, offset + camera_bytes, 1, 1};
    context->CopySubresourceRegion(staging_.Get(), 0, 0, 0, 0, bound.buffer.Get(), 0, &box);
    pending_ = true;
    return true;
}

std::optional<CameraMatrices> CameraReadback::resolve(ID3D11DeviceContext* context) noexcept {
    if (!pending_ || !staging_) return std::nullopt;
    pending_ = false;
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging_.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    const auto* values = static_cast<const float*>(mapped.pData);
    CameraMatrices matrices;
    std::copy(values, values + 16, matrices.view.begin());
    std::copy(values + 16, values + camera_floats, matrices.view_projection.begin());
    context->Unmap(staging_.Get(), 0);
    return matrices;
}
}
