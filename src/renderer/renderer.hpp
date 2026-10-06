#pragma once

#include "config/config.hpp"
#include "renderer/resource_manager.hpp"
#include "renderer/gpu_profiler.hpp"
#include "renderer/lut_resource.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <wrl/client.h>

namespace neuralfx {
class Renderer {
public:
    static Renderer& instance() noexcept;
    void present(IDXGISwapChain* swapchain);
    void reset(IDXGISwapChain* swapchain) noexcept;
    void set_neural_residual(ID3D11ShaderResourceView* resource) noexcept;

private:
    HRESULT initialize(IDXGISwapChain* swapchain);
    HRESULT initialize_photoreal();
    void ensure_initialized(IDXGISwapChain* swapchain);
    void reset_unlocked() noexcept;
    HRESULT compile_shaders();
    void draw(ID3D11PixelShader* pixel, ID3D11ShaderResourceView* input,
        ID3D11RenderTargetView* output, ID3D11ShaderResourceView* neural = nullptr,
        ID3D11ShaderResourceView* lut = nullptr, ID3D11Buffer* parameters = nullptr);
    void render_photoreal(ID3D11ShaderResourceView* input);
    void render_screen_space(ID3D11ShaderResourceView* depth);
    void poll_hotkey() noexcept;
    void poll_config();
    void load_lut();

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
    Microsoft::WRL::ComPtr<ID3D11PixelShader> photoreal_pixel_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> finish_pixel_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> trace_pixel_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> trace_constants_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> neural_residual_;
    LutResource lut_;
    float frame_index_ = 0.0f;
    std::filesystem::file_time_type config_write_{};
    std::chrono::steady_clock::time_point next_config_poll_{};
};
}
