#include "upscale/dx12/shared_texture.hpp"

#include <dxgi1_2.h>

namespace neuralfx::upscale {
std::optional<SharedTexture> create_shared_texture(ID3D11Device* host, ID3D12Device* device, DXGI_FORMAT format,
    UINT width, UINT height, UINT extra_bind_flags) noexcept {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = format;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | extra_bind_flags;
    description.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    SharedTexture shared;
    shared.format = format;
    shared.width = width;
    shared.height = height;
    if (FAILED(host->CreateTexture2D(&description, nullptr, &shared.host))) return std::nullopt;
    Microsoft::WRL::ComPtr<IDXGIResource1> resource;
    HANDLE handle = nullptr;
    if (FAILED(shared.host.As(&resource))) return std::nullopt;
    if (FAILED(resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr,
            &handle))) return std::nullopt;
    HRESULT opened = device->OpenSharedHandle(handle, IID_PPV_ARGS(&shared.device));
    CloseHandle(handle);
    if (FAILED(opened)) return std::nullopt;
    return shared;
}

bool matches(const SharedTexture& shared, ID3D11Texture2D* texture) noexcept {
    if (!texture) return false;
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    return description.Format == shared.format && description.Width == shared.width &&
        description.Height == shared.height && description.SampleDesc.Count == 1;
}
}
