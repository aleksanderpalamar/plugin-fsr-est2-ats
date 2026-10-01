#include <d3dcompiler.h>
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <wrl/client.h>

namespace {
bool compile(const std::filesystem::path& path, const char* entry, const char* profile) {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    Microsoft::WRL::ComPtr<ID3DBlob> messages;
    HRESULT result = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry, profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS,
        0, &blob, &messages);
    if (messages) std::cerr.write(static_cast<const char*>(messages->GetBufferPointer()), messages->GetBufferSize());
    return SUCCEEDED(result);
}
}

int main() {
    wchar_t executable[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, executable, MAX_PATH) == 0) return 1;
    auto directory = std::filesystem::path(executable).parent_path() / L"NeuralFX" / L"shaders";
    auto fullscreen = directory / L"fullscreen.hlsl";
    auto photoreal = directory / L"photoreal.hlsl";
    if (!compile(fullscreen, "VSMain", "vs_5_0")) return 2;
    if (!compile(fullscreen, "CopyMain", "ps_5_0")) return 3;
    if (!compile(fullscreen, "RcasMain", "ps_5_0")) return 4;
    if (!compile(photoreal, "PhotorealMain", "ps_5_0")) return 5;
    if (!compile(photoreal, "FinishMain", "ps_5_0")) return 6;
    return 0;
}
