#include "renderer/lut_resource.hpp"
#include "renderer/lut_data.hpp"

#include <fstream>

namespace neuralfx {
HRESULT LutResource::load(ID3D11Device* device, const std::filesystem::path& path) {
    reset();
    if (!device) return E_POINTER;
    std::ifstream file(path);
    if (!file) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    auto data = parse_lut(file);
    if (!data) return E_INVALIDARG;
    D3D11_TEXTURE3D_DESC description{};
    description.Width = data->size;
    description.Height = data->size;
    description.Depth = data->size;
    description.MipLevels = 1;
    description.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    description.Usage = D3D11_USAGE_IMMUTABLE;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial{};
    initial.pSysMem = data->texels.data();
    initial.SysMemPitch = data->size * sizeof(data->texels[0]);
    initial.SysMemSlicePitch = data->size * data->size * sizeof(data->texels[0]);
    HRESULT result = device->CreateTexture3D(&description, &initial, &texture_);
    if (FAILED(result)) return result;
    result = device->CreateShaderResourceView(texture_.Get(), nullptr, &view_);
    if (FAILED(result)) return result;
    size_ = data->size;
    return S_OK;
}

void LutResource::reset() noexcept {
    view_.Reset();
    texture_.Reset();
    size_ = 0;
}
}
