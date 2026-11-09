#include "temporal/camera_capture.hpp"

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

ComPtr<ID3D11Buffer> create_buffer(ID3D11DeviceContext* context, D3D11_USAGE usage, UINT bind, UINT access) {
    ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    D3D11_BUFFER_DESC description{camera_bytes, usage, bind, access, 0, 0};
    ComPtr<ID3D11Buffer> buffer;
    if (device) device->CreateBuffer(&description, nullptr, &buffer);
    return buffer;
}

CameraMatrices matrices_of(const void* data) {
    const auto* values = static_cast<const float*>(data);
    CameraMatrices matrices;
    std::copy(values, values + 16, matrices.view.begin());
    std::copy(values + 16, values + camera_floats, matrices.view_projection.begin());
    return matrices;
}
}

bool CameraCapture::capture(ID3D11DeviceContext* context, std::uint64_t frame) noexcept {
    auto bound = bound_camera(context);
    if (!bound.buffer || !holds_camera(bound)) return false;
    auto& slot = pending_[frame % pending_.size()];
    if (!prepare(context, slot)) return false;
    UINT offset = bound.first * constant_bytes;
    D3D11_BOX box{offset, 0, 0, offset + camera_bytes, 1, 1};
    context->CopySubresourceRegion(camera_.Get(), 0, 0, 0, 0, bound.buffer.Get(), 0, &box);
    context->CopyResource(slot.staging.Get(), camera_.Get());
    slot.frame = frame;
    return true;
}

std::optional<CameraObservation> CameraCapture::collect(ID3D11DeviceContext* context) noexcept {
    std::optional<CameraObservation> latest;
    while (auto* slot = oldest()) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT result = context->Map(slot->staging.Get(), 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);
        if (result == DXGI_ERROR_WAS_STILL_DRAWING) break;
        auto frame = *slot->frame;
        slot->frame.reset();
        if (FAILED(result)) continue;
        latest = CameraObservation{frame, matrices_of(mapped.pData)};
        context->Unmap(slot->staging.Get(), 0);
    }
    return latest;
}

bool CameraCapture::prepare(ID3D11DeviceContext* context, Pending& slot) noexcept {
    if (!camera_) camera_ = create_buffer(context, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0);
    if (!slot.staging) slot.staging = create_buffer(context, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ);
    return camera_ && slot.staging;
}

CameraCapture::Pending* CameraCapture::oldest() noexcept {
    Pending* found = nullptr;
    for (auto& slot : pending_)
        if (slot.frame && (!found || *slot.frame < *found->frame)) found = &slot;
    return found;
}
}
