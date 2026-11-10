#pragma once

#include "temporal/projection_jitter.hpp"

#include <d3d11.h>
#include <wrl/client.h>

namespace neuralfx {
class MotionTargets {
public:
    bool prepare(ID3D11Device* device, ID3D11Texture2D* scene_depth, temporal::RenderSize size) noexcept;
    void copy_depth(ID3D11DeviceContext* context, ID3D11Texture2D* scene_depth) noexcept;
    ID3D11ShaderResourceView* depth() const noexcept { return depth_view_.Get(); }
    ID3D11RenderTargetView* motion_target() const noexcept { return motion_target_.Get(); }
    ID3D11ShaderResourceView* motion() const noexcept { return motion_view_.Get(); }
    ID3D11Texture2D* motion_texture() const noexcept { return motion_.Get(); }

private:
    bool create_depth(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& source) noexcept;
    bool create_motion(ID3D11Device* device, temporal::RenderSize size) noexcept;

    D3D11_TEXTURE2D_DESC source_{};
    temporal::RenderSize size_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depth_view_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> motion_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> motion_target_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> motion_view_;
};
}
