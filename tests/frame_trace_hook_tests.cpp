#include "diagnostics/frame_tracer.hpp"
#include "dxgi/context_hook.hpp"

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

namespace {
using Microsoft::WRL::ComPtr;

struct Device {
    ComPtr<IDXGISwapChain> swapchain;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
};

HRESULT create_device(HWND window, Device& output) {
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferDesc.Width = 32;
    description.BufferDesc.Height = 32;
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

ComPtr<ID3D11Texture2D> create_texture(ID3D11Device* device, UINT size, DXGI_FORMAT format, UINT bind) {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = size;
    description.Height = size;
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

constexpr char shader_source[] =
    "float4 VSMain(uint id : SV_VertexID) : SV_Position { return float4(0, 0, 0, 1); }\n"
    "float4 PSMain(float4 position : SV_Position) : SV_Target { return 0; }\n";

ComPtr<ID3DBlob> compile(const char* entry, const char* target) {
    ComPtr<ID3DBlob> code;
    D3DCompile(shader_source, sizeof(shader_source) - 1, nullptr, nullptr, nullptr, entry, target, 0, 0,
        &code, nullptr);
    return code;
}

bool bind_pipeline(ID3D11Device* device, ID3D11DeviceContext* context) {
    auto vertex_code = compile("VSMain", "vs_5_0");
    auto pixel_code = compile("PSMain", "ps_5_0");
    if (!vertex_code || !pixel_code) return false;
    ComPtr<ID3D11VertexShader> vertex;
    ComPtr<ID3D11PixelShader> pixel;
    if (FAILED(device->CreateVertexShader(vertex_code->GetBufferPointer(), vertex_code->GetBufferSize(),
        nullptr, &vertex))) return false;
    if (FAILED(device->CreatePixelShader(pixel_code->GetBufferPointer(), pixel_code->GetBufferSize(),
        nullptr, &pixel))) return false;
    const UINT indices[3]{0, 1, 2};
    D3D11_BUFFER_DESC description{sizeof(indices), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA data{indices, 0, 0};
    ComPtr<ID3D11Buffer> index_buffer;
    if (FAILED(device->CreateBuffer(&description, &data, &index_buffer))) return false;
    context->VSSetShader(vertex.Get(), nullptr, 0);
    context->PSSetShader(pixel.Get(), nullptr, 0);
    context->IASetIndexBuffer(index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    return true;
}

std::string hex(const void* pointer) {
    char buffer[24]{};
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), reinterpret_cast<std::uintptr_t>(pointer), 16);
    return std::string(buffer, result.ptr);
}

bool contains(const std::vector<std::string>& lines, const std::string& expected) {
    if (std::find(lines.begin(), lines.end(), expected) != lines.end()) return true;
    std::cerr << "missing: " << expected << '\n';
    return false;
}

void game_frame(Device& gpu, ID3D11RenderTargetView* scene, ID3D11DepthStencilView* depth,
    ID3D11ShaderResourceView* scene_input, ID3D11RenderTargetView* output, ID3D11Texture2D* history,
    ID3D11Texture2D* scene_texture) {
    const float black[4]{};
    auto* context = gpu.context.Get();
    context->ClearRenderTargetView(scene, black);
    context->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, 1.0f, 0);
    context->OMSetRenderTargets(1, &scene, depth);
    D3D11_VIEWPORT viewport{0.0f, 0.0f, 16.0f, 16.0f, 0.0f, 1.0f};
    context->RSSetViewports(1, &viewport);
    context->Draw(3, 0);
    context->DrawIndexed(3, 0, 0);
    context->CopyResource(history, scene_texture);
    context->OMSetRenderTargets(1, &output, nullptr);
    context->PSSetShaderResources(0, 1, &scene_input);
    context->DrawInstanced(3, 1, 0, 0);
}
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW,
        0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return 1;
    Device gpu;
    if (FAILED(create_device(window, gpu))) return 2;
    auto* device = gpu.device.Get();
    auto scene_texture = create_texture(device, 16, DXGI_FORMAT_R16G16_FLOAT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    auto history = create_texture(device, 16, DXGI_FORMAT_R16G16_FLOAT, D3D11_BIND_SHADER_RESOURCE);
    auto depth_texture = create_texture(device, 16, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
    ComPtr<ID3D11Texture2D> backbuffer;
    if (!scene_texture || !history || !depth_texture) return 3;
    if (FAILED(gpu.swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)))) return 4;
    ComPtr<ID3D11RenderTargetView> scene, output;
    ComPtr<ID3D11DepthStencilView> depth;
    ComPtr<ID3D11ShaderResourceView> scene_input;
    if (FAILED(device->CreateRenderTargetView(scene_texture.Get(), nullptr, &scene))) return 5;
    if (FAILED(device->CreateRenderTargetView(backbuffer.Get(), nullptr, &output))) return 6;
    if (FAILED(device->CreateDepthStencilView(depth_texture.Get(), nullptr, &depth))) return 7;
    if (FAILED(device->CreateShaderResourceView(scene_texture.Get(), nullptr, &scene_input))) return 8;
    if (!bind_pipeline(device, gpu.context.Get())) return 14;
    neuralfx::hook_context(gpu.swapchain.Get());
    namespace trace = neuralfx::diagnostics;
    if (!trace::trace_present_begin().empty()) return 9;
    trace::trace_present_end(gpu.swapchain.Get());
    game_frame(gpu, scene.Get(), depth.Get(), scene_input.Get(), output.Get(), history.Get(), scene_texture.Get());
    if (!trace::trace_present_begin().empty()) return 10;
    if (!trace::request_capture(1)) return 11;
    trace::trace_present_end(gpu.swapchain.Get());
    game_frame(gpu, scene.Get(), depth.Get(), scene_input.Get(), output.Get(), history.Get(), scene_texture.Get());
    auto lines = trace::trace_present_begin();
    auto scene_id = hex(scene_texture.Get());
    auto depth_id = hex(depth_texture.Get());
    auto history_id = hex(history.Get());
    bool valid = contains(lines, "Trace frame begin backbuffer=bb 32x32 fmt=28 samples=1") &&
        contains(lines, "Trace 0 clear-color rt=" + scene_id) &&
        contains(lines, "Trace 1 clear-depth ds=" + depth_id) &&
        contains(lines, "Trace 2 pass rt=" + scene_id + " ds=" + depth_id + " vp=16x16 draws=2") &&
        contains(lines, "Trace 3 copy src=" + scene_id + " dst=" + history_id) &&
        contains(lines, "Trace 4 pass rt=bb draws=1 reads=" + scene_id) &&
        contains(lines, "Trace target " + scene_id + " 16x16 fmt=34 samples=1 color writes=1 reads=2") &&
        contains(lines, "Trace target " + depth_id + " 16x16 fmt=45 samples=1 depth writes=1 reads=0") &&
        contains(lines, "Trace frame end entries=5 draws=3 dropped=0");
    if (!valid) {
        for (const auto& line : lines) std::cerr << line << '\n';
        return 12;
    }
    trace::trace_present_end(gpu.swapchain.Get());
    game_frame(gpu, scene.Get(), depth.Get(), scene_input.Get(), output.Get(), history.Get(), scene_texture.Get());
    if (!trace::trace_present_begin().empty()) return 13;
    DestroyWindow(window);
    return 0;
}
