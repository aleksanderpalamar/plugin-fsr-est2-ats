#include "renderer/renderer.hpp"
#include "renderer/context_state.hpp"
#include "common/logger.hpp"

#include <windows.h>
#include <d3dcompiler.h>
#include <string>

namespace neuralfx {
namespace {
HRESULT compile(const std::filesystem::path& path, const char* entry, const char* profile, ID3DBlob** blob) {
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    HRESULT result = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry, profile, D3DCOMPILE_ENABLE_STRICTNESS, 0, blob, &errors);
    if (FAILED(result) && errors) log(std::string_view(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize()));
    return result;
}
}

Renderer& Renderer::instance() noexcept {
    static Renderer renderer;
    return renderer;
}

void Renderer::poll_hotkey() noexcept {
    bool down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
    if (down && !key_down_) config_.enabled = !config_.enabled;
    key_down_ = down;
}

void Renderer::present(IDXGISwapChain* swapchain) {
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock() || !swapchain) return;
    if (active_swapchain_ != swapchain) {
        reset_unlocked();
        active_swapchain_ = swapchain;
    }
    if (!initialized_ && !failed_) ensure_initialized(swapchain);
    poll_hotkey();
    if (!initialized_ || !config_.enabled || config_.mode == TestMode::Hook) return;
    profiler_.poll(context_.Get());
    profiler_.begin(context_.Get());
    ContextState previous(context_.Get());
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    context_->CopyResource(resources_.copy(), resources_.backbuffer());
    if (config_.mode == TestMode::Copy) {
        context_->CopyResource(resources_.backbuffer(), resources_.copy());
        profiler_.end(context_.Get());
        return;
    }
    draw(config_.mode == TestMode::Rcas ? rcas_pixel_.Get() : copy_pixel_.Get());
    profiler_.end(context_.Get());
}

void Renderer::reset(IDXGISwapChain* swapchain) noexcept {
    std::lock_guard lock(mutex_);
    if (swapchain && active_swapchain_ != swapchain) return;
    reset_unlocked();
}

void Renderer::reset_unlocked() noexcept {
    resources_.reset();
    profiler_.reset();
    constants_.Reset();
    rcas_pixel_.Reset();
    copy_pixel_.Reset();
    vertex_.Reset();
    context_.Reset();
    device_.Reset();
    initialized_ = false;
    failed_ = false;
    active_swapchain_ = nullptr;
}

void Renderer::ensure_initialized(IDXGISwapChain* swapchain) {
    config_ = load_config();
    HRESULT result = initialize(swapchain);
    failed_ = FAILED(result);
    initialized_ = !failed_;
    if (failed_) log("Renderer initialization failed: " + std::to_string(static_cast<unsigned long>(result)));
}

HRESULT Renderer::initialize(IDXGISwapChain* swapchain) {
    HRESULT result = swapchain->GetDevice(IID_PPV_ARGS(&device_));
    if (FAILED(result)) return result;
    device_->GetImmediateContext(&context_);
    if (!context_) return E_FAIL;
    result = resources_.initialize(device_.Get(), swapchain);
    if (FAILED(result)) return result;
    result = compile_shaders();
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = 16;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    result = device_->CreateBuffer(&description, nullptr, &constants_);
    if (SUCCEEDED(result) && FAILED(profiler_.initialize(device_.Get()))) log("GPU timing unavailable");
    if (SUCCEEDED(result)) log("D3D11 renderer ready: " + std::to_string(resources_.width()) + "x" + std::to_string(resources_.height()));
    if (SUCCEEDED(result)) log("UI isolation unavailable; post-processing affects HUD");
    return result;
}

HRESULT Renderer::compile_shaders() {
    auto path = module_directory() / L"NeuralFX" / L"shaders" / L"fullscreen.hlsl";
    Microsoft::WRL::ComPtr<ID3DBlob> vertex_blob;
    Microsoft::WRL::ComPtr<ID3DBlob> copy_blob;
    Microsoft::WRL::ComPtr<ID3DBlob> rcas_blob;
    HRESULT result = compile(path, "VSMain", "vs_5_0", vertex_blob.GetAddressOf());
    if (FAILED(result)) return result;
    result = compile(path, "CopyMain", "ps_5_0", copy_blob.GetAddressOf());
    if (FAILED(result)) return result;
    result = compile(path, "RcasMain", "ps_5_0", rcas_blob.GetAddressOf());
    if (FAILED(result)) return result;
    result = device_->CreateVertexShader(vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize(), nullptr, &vertex_);
    if (FAILED(result)) return result;
    result = device_->CreatePixelShader(copy_blob->GetBufferPointer(), copy_blob->GetBufferSize(), nullptr, &copy_pixel_);
    if (FAILED(result)) return result;
    return device_->CreatePixelShader(rcas_blob->GetBufferPointer(), rcas_blob->GetBufferSize(), nullptr, &rcas_pixel_);
}

void Renderer::draw(ID3D11PixelShader* pixel) {
    D3D11_VIEWPORT viewport{0, 0, static_cast<float>(resources_.width()), static_cast<float>(resources_.height()), 0, 1};
    D3D11_RECT scissor{0, 0, static_cast<LONG>(resources_.width()), static_cast<LONG>(resources_.height())};
    struct Parameters { float amount; float padding[3]; } parameters{config_.sharpness, {0, 0, 0}};
    context_->UpdateSubresource(constants_.Get(), 0, nullptr, &parameters, 0, 0);
    ID3D11RenderTargetView* target = resources_.output();
    ID3D11ShaderResourceView* input = resources_.input();
    ID3D11Buffer* constants = constants_.Get();
    context_->OMSetRenderTargets(1, &target, nullptr);
    context_->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    context_->OMSetDepthStencilState(nullptr, 0);
    context_->RSSetState(nullptr);
    context_->RSSetViewports(1, &viewport);
    context_->RSSetScissorRects(1, &scissor);
    context_->IASetInputLayout(nullptr);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vertex_.Get(), nullptr, 0);
    context_->HSSetShader(nullptr, nullptr, 0);
    context_->DSSetShader(nullptr, nullptr, 0);
    context_->GSSetShader(nullptr, nullptr, 0);
    context_->PSSetShader(pixel, nullptr, 0);
    context_->PSSetShaderResources(0, 1, &input);
    context_->PSSetConstantBuffers(0, 1, &constants);
    context_->Draw(3, 0);
    ID3D11ShaderResourceView* empty = nullptr;
    context_->PSSetShaderResources(0, 1, &empty);
}
}
