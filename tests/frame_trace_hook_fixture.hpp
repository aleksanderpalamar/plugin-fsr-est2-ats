#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <wrl/client.h>
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace trace_test {
using Microsoft::WRL::ComPtr;

struct Device {
    ComPtr<IDXGISwapChain> swapchain;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
};

struct Pipeline {
    ComPtr<ID3D11VertexShader> vertex;
    ComPtr<ID3D11PixelShader> pixel;
    ComPtr<ID3D11Buffer> indices;
    ComPtr<ID3D11Buffer> constants;
};

inline HRESULT create_device(HWND window, Device& output, UINT width = 32, UINT height = 32) {
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferDesc.Width = width;
    description.BufferDesc.Height = height;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.OutputWindow = window;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    auto create = [&](D3D_DRIVER_TYPE type) {
        return D3D11CreateDeviceAndSwapChain(nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
            &description, &output.swapchain, &output.device, nullptr, &output.context);
    };
    HRESULT result = create(D3D_DRIVER_TYPE_HARDWARE);
    return SUCCEEDED(result) ? result : create(D3D_DRIVER_TYPE_WARP);
}

inline ComPtr<ID3D11Texture2D> create_texture(ID3D11Device* device, DXGI_FORMAT format, UINT bind) {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = 32;
    description.Height = 32;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = format;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = bind;
    ComPtr<ID3D11Texture2D> texture;
    device->CreateTexture2D(&description, nullptr, &texture);
    return texture;
}

inline ComPtr<ID3DBlob> compile(const char* entry, const char* target) {
    constexpr char source[] =
        "float4 VSMain(uint id : SV_VertexID) : SV_Position { return float4(0, 0, 0, 1); }\n"
        "float4 PSMain(float4 position : SV_Position) : SV_Target { return 0; }\n";
    ComPtr<ID3DBlob> code;
    D3DCompile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, entry, target, 0, 0, &code, nullptr);
    return code;
}

inline bool create_pipeline(ID3D11Device* device, Pipeline& pipeline) {
    auto vertex_code = compile("VSMain", "vs_5_0");
    auto pixel_code = compile("PSMain", "ps_5_0");
    if (!vertex_code || !pixel_code) return false;
    if (FAILED(device->CreateVertexShader(vertex_code->GetBufferPointer(), vertex_code->GetBufferSize(),
        nullptr, &pipeline.vertex))) return false;
    if (FAILED(device->CreatePixelShader(pixel_code->GetBufferPointer(), pixel_code->GetBufferSize(),
        nullptr, &pipeline.pixel))) return false;
    const UINT indices[3]{0, 1, 2};
    D3D11_BUFFER_DESC index{sizeof(indices), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA data{indices, 0, 0};
    if (FAILED(device->CreateBuffer(&index, &data, &pipeline.indices))) return false;
    D3D11_BUFFER_DESC constants{16, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
    return SUCCEEDED(device->CreateBuffer(&constants, nullptr, &pipeline.constants));
}

inline void bind_pipeline(ID3D11DeviceContext* context, const Pipeline& pipeline) {
    context->VSSetShader(pipeline.vertex.Get(), nullptr, 0);
    context->PSSetShader(pipeline.pixel.Get(), nullptr, 0);
    context->IASetIndexBuffer(pipeline.indices.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

inline void write_constants(ID3D11DeviceContext* context, ID3D11Buffer* buffer) {
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
    const float values[4]{1.0f, 2.0f, 3.0f, 4.0f};
    std::copy(values, values + 4, static_cast<float*>(mapped.pData));
    context->Unmap(buffer, 0);
}

inline std::string hex(const void* pointer) {
    char buffer[24]{};
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), reinterpret_cast<std::uintptr_t>(pointer), 16);
    return std::string(buffer, result.ptr);
}

inline bool same_lines(const std::vector<std::string>& lines, const std::vector<std::string>& expected) {
    if (lines == expected) return true;
    std::cerr << "produced:\n";
    for (const auto& line : lines) std::cerr << line << '\n';
    std::cerr << "expected:\n";
    for (const auto& line : expected) std::cerr << line << '\n';
    return false;
}
}
