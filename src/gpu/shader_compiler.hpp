#pragma once

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace neuralfx {
HRESULT create_vertex_shader(ID3D11Device* device, const std::filesystem::path& path,
    const char* entry, Microsoft::WRL::ComPtr<ID3D11VertexShader>& shader);
HRESULT create_pixel_shader(ID3D11Device* device, const std::filesystem::path& path,
    const char* entry, Microsoft::WRL::ComPtr<ID3D11PixelShader>& shader);
HRESULT create_compute_shader(ID3D11Device* device, const std::filesystem::path& path,
    const char* entry, Microsoft::WRL::ComPtr<ID3D11ComputeShader>& shader);
}
