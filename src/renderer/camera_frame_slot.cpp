#include "renderer/camera_frame_slot.hpp"

namespace neuralfx {
HRESULT CameraFrameSlot::create(ID3D11Device* device) {
    D3D11_BUFFER_DESC description{sizeof(GpuCameraFrame), D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        sizeof(GpuCameraFrame)};
    GpuCameraFrame empty{};
    D3D11_SUBRESOURCE_DATA data{&empty, 0, 0};
    HRESULT result = device->CreateBuffer(&description, &data, &buffer);
    if (FAILED(result)) return result;
    result = device->CreateShaderResourceView(buffer.Get(), nullptr, &view);
    if (FAILED(result)) return result;
    return device->CreateUnorderedAccessView(buffer.Get(), nullptr, &output);
}
}
