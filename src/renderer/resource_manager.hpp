#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace neuralfx {
class RenderResourceManager {
public:
    HRESULT initialize(ID3D11Device* device, IDXGISwapChain* swapchain,
        bool photoreal, bool raytracing) noexcept;
    void reset() noexcept;
    ID3D11Texture2D* backbuffer() const noexcept { return backbuffer_.Get(); }
    ID3D11RenderTargetView* output() const noexcept { return output_.Get(); }
    ID3D11ShaderResourceView* input() const noexcept { return input_.Get(); }
    ID3D11ShaderResourceView* stage_input() const noexcept { return stage_input_.Get(); }
    ID3D11RenderTargetView* stage_output() const noexcept { return stage_output_.Get(); }
    ID3D11ShaderResourceView* traced_input() const noexcept { return traced_input_.Get(); }
    ID3D11RenderTargetView* traced_output() const noexcept { return traced_output_.Get(); }
    ID3D11Texture2D* copy() const noexcept { return copy_.Get(); }
    UINT width() const noexcept { return width_; }
    UINT height() const noexcept { return height_; }
    DXGI_FORMAT format() const noexcept { return format_; }
    bool needs_srgb_conversion() const noexcept { return format_ != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB && format_ != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; }

private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backbuffer_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> copy_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> stage_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> traced_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> traced_input_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> traced_output_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> stage_input_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> stage_output_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> output_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> input_;
    UINT width_ = 0;
    UINT height_ = 0;
    DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;
};
}
