#include "gpu/shader_compiler.hpp"
#include "common/logger.hpp"

#include <d3dcompiler.h>
#include <string_view>

namespace neuralfx {
namespace {
HRESULT compile(const std::filesystem::path& path, const char* entry,
    const char* profile, ID3DBlob** blob) {
    Microsoft::WRL::ComPtr<ID3DBlob> messages;
    HRESULT result = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry, profile, D3DCOMPILE_ENABLE_STRICTNESS, 0, blob, &messages);
    if (messages && messages->GetBufferSize() > 0)
        log(std::string_view(static_cast<const char*>(messages->GetBufferPointer()), messages->GetBufferSize()));
    return result;
}
}

HRESULT create_vertex_shader(ID3D11Device* device, const std::filesystem::path& path,
    const char* entry, Microsoft::WRL::ComPtr<ID3D11VertexShader>& shader) {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    HRESULT result = compile(path, entry, "vs_5_0", &blob);
    if (FAILED(result)) return result;
    return device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
}

HRESULT create_pixel_shader(ID3D11Device* device, const std::filesystem::path& path,
    const char* entry, Microsoft::WRL::ComPtr<ID3D11PixelShader>& shader) {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    HRESULT result = compile(path, entry, "ps_5_0", &blob);
    if (FAILED(result)) return result;
    return device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
}
}
