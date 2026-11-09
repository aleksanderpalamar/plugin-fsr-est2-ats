#pragma once

#include <d3d11_4.h>
#include <wrl/client.h>
#include <optional>
#include <vector>

namespace spike {
using Microsoft::WRL::ComPtr;

struct D3D11Source {
    ComPtr<ID3D11Device5> device;
    ComPtr<ID3D11DeviceContext4> context;
    ComPtr<ID3D11Fence> fence;
    HANDLE fence_handle = nullptr;
    LUID adapter{};
    UINT64 fence_value = 0;
};

struct SharedTexture {
    ComPtr<ID3D11Texture2D> texture;
    HANDLE handle = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    UINT width = 0;
    UINT height = 0;
};

std::optional<D3D11Source> create_source();
std::optional<SharedTexture> create_shared(D3D11Source& source, const char* name, DXGI_FORMAT format,
    UINT width, UINT height, const std::vector<unsigned char>& pixels, UINT pitch);
UINT64 signal(D3D11Source& source);
}
