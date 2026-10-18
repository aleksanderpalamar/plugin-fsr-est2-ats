#include "raytracing/gpu_scene_buffers.hpp"

#include <algorithm>
#include <limits>

namespace neuralfx::raytracing {
namespace {
template <typename Element>
HRESULT create_structured(ID3D11Device* device, const std::vector<Element>& elements,
    Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& view) {
    const Element placeholder{};
    size_t count = std::max<size_t>(elements.size(), 1);
    if (count > std::numeric_limits<UINT>::max() / sizeof(Element)) return E_INVALIDARG;
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = static_cast<UINT>(count * sizeof(Element));
    description.Usage = D3D11_USAGE_IMMUTABLE;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    description.StructureByteStride = sizeof(Element);
    D3D11_SUBRESOURCE_DATA initial{elements.empty() ? &placeholder : elements.data(), 0, 0};
    HRESULT result = device->CreateBuffer(&description, &initial, &buffer);
    if (FAILED(result)) return result;
    D3D11_SHADER_RESOURCE_VIEW_DESC view_description{};
    view_description.Format = DXGI_FORMAT_UNKNOWN;
    view_description.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    view_description.Buffer.FirstElement = 0;
    view_description.Buffer.NumElements = static_cast<UINT>(count);
    return device->CreateShaderResourceView(buffer.Get(), &view_description, &view);
}
}

HRESULT GpuSceneBuffers::upload(ID3D11Device* device, const GpuSceneData& scene) {
    reset();
    if (!device) return E_POINTER;
    std::array<HRESULT, view_count> results{
        create_structured(device, scene.nodes, buffers_[0], views_[0]),
        create_structured(device, scene.triangles, buffers_[1], views_[1]),
        create_structured(device, scene.materials, buffers_[2], views_[2]),
        create_structured(device, scene.lights, buffers_[3], views_[3])
    };
    auto failure = std::find_if(results.begin(), results.end(), [](HRESULT result) { return FAILED(result); });
    if (failure != results.end()) {
        HRESULT result = *failure;
        reset();
        return result;
    }
    counts_ = scene_counts(scene);
    ready_ = true;
    return S_OK;
}

void GpuSceneBuffers::reset() noexcept {
    for (auto& view : views_) view.Reset();
    for (auto& buffer : buffers_) buffer.Reset();
    counts_ = {};
    ready_ = false;
}

std::array<ID3D11ShaderResourceView*, GpuSceneBuffers::view_count> GpuSceneBuffers::views() const noexcept {
    std::array<ID3D11ShaderResourceView*, view_count> result{};
    for (size_t index = 0; index < view_count; ++index) result[index] = views_[index].Get();
    return result;
}
}
