#include "temporal/camera_capture.hpp"
#include "temporal/bound_constants.hpp"

#include <algorithm>

namespace neuralfx::temporal {
namespace {
constexpr UINT camera_floats = 32;
constexpr UINT camera_bytes = camera_floats * sizeof(float);

CameraMatrices matrices_of(const std::vector<float>& values) {
    CameraMatrices matrices;
    std::copy(values.begin(), values.begin() + 16, matrices.view.begin());
    std::copy(values.begin() + 16, values.begin() + camera_floats, matrices.view_projection.begin());
    return matrices;
}
}

CameraCapture::CameraCapture() noexcept : ring_(camera_bytes) {}

bool CameraCapture::capture(ID3D11DeviceContext* context, std::uint64_t frame) noexcept {
    auto bound = bound_constants(context, ConstantStage::Vertex, 0);
    if (!holds(bound, camera_bytes) || !prepare(context)) return false;
    D3D11_BOX box{bound.offset_bytes, 0, 0, bound.offset_bytes + camera_bytes, 1, 1};
    context->CopySubresourceRegion(camera_.Get(), 0, 0, 0, 0, bound.buffer.Get(), 0, &box);
    return ring_.copy(context, frame, camera_.Get(), 0);
}

std::vector<CameraObservation> CameraCapture::collect(ID3D11DeviceContext* context) noexcept {
    std::vector<CameraObservation> completed;
    try {
        for (const auto& read : ring_.collect(context)) completed.push_back({read.frame, matrices_of(read.values)});
    } catch (...) {
    }
    return completed;
}

bool CameraCapture::prepare(ID3D11DeviceContext* context) noexcept {
    if (camera_) return true;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    D3D11_BUFFER_DESC description{camera_bytes, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    return device && SUCCEEDED(device->CreateBuffer(&description, nullptr, &camera_));
}
}
