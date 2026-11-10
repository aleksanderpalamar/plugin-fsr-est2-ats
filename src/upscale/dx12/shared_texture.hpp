#pragma once

#include <d3d11_4.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <optional>

namespace neuralfx::upscale {
struct SharedTexture {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> host;
    Microsoft::WRL::ComPtr<ID3D12Resource> device;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    UINT width = 0;
    UINT height = 0;
};

std::optional<SharedTexture> create_shared_texture(ID3D11Device* host, ID3D12Device* device, DXGI_FORMAT format,
    UINT width, UINT height, UINT extra_bind_flags) noexcept;
bool matches(const SharedTexture& shared, ID3D11Texture2D* texture) noexcept;
}
