#pragma once

#include "screenshots/rgb_image.hpp"

#include <d3d11.h>
#include <wrl/client.h>

namespace neuralfx {
class D3D11ScreenshotReadback {
public:
    HRESULT prepare(ID3D11Texture2D* texture) noexcept;
    std::optional<RgbImage> capture(ID3D11Texture2D* texture);
    void reset() noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_;
    D3D11_TEXTURE2D_DESC description_{};
};
}
