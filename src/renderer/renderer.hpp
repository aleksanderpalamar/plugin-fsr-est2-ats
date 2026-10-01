#pragma once

#include "config/config.hpp"
#include "renderer/resource_manager.hpp"
#include "renderer/gpu_profiler.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <mutex>
#include <wrl/client.h>

namespace neuralfx {
class Renderer {
public:
    static Renderer& instance() noexcept;
    void present(IDXGISwapChain* swapchain);
    void reset(IDXGISwapChain* swapchain) noexcept;

private:
    HRESULT initialize(IDXGISwapChain* swapchain);
    void ensure_initialized(IDXGISwapChain* swapchain);
    void reset_unlocked() noexcept;
    HRESULT compile_shaders();
    void draw(ID3D11PixelShader* pixel);
    void poll_hotkey() noexcept;

    std::mutex mutex_;
    IDXGISwapChain* active_swapchain_ = nullptr;
    bool initialized_ = false;
    bool failed_ = false;
    bool key_down_ = false;
    Config config_;
    RenderResourceManager resources_;
    GpuProfiler profiler_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> copy_pixel_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> rcas_pixel_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;
};
}
