#pragma once

#include <d3d11.h>
#include <atomic>
#include <mutex>
#include <wrl/client.h>

namespace neuralfx {
class DepthCapture {
public:
    static DepthCapture& instance() noexcept;
    void configure(ID3D11Device* device, UINT width, UINT height) noexcept;
    void reset() noexcept;
    void set_active(bool active) noexcept { active_.store(active, std::memory_order_relaxed); }
    bool active() const noexcept { return active_.load(std::memory_order_relaxed); }
    void capture(ID3D11DeviceContext* context, ID3D11DepthStencilView* depth) noexcept;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> take_view() noexcept;

private:
    bool prepare(const D3D11_TEXTURE2D_DESC& description,
        DXGI_FORMAT texture_format, DXGI_FORMAT view_format) noexcept;

    std::mutex mutex_;
    std::atomic_bool active_{false};
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> copy_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_;
    UINT width_ = 0;
    UINT height_ = 0;
    DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;
    bool ready_ = false;
};
}
