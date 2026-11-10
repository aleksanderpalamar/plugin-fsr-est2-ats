#pragma once

#include "renderer/camera_frame.hpp"

#include <d3d11.h>
#include <wrl/client.h>

namespace neuralfx {
struct CameraFrameSlot {
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> output;

    HRESULT create(ID3D11Device* device);
};
}
