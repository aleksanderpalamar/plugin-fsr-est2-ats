#pragma once

#include "renderer/camera_motion_pass.hpp"
#include "renderer/depth_to_float_pass.hpp"
#include "renderer/motion_targets.hpp"
#include "renderer/reactive_mask_pass.hpp"
#include "renderer/resolve_frame_pass.hpp"
#include "upscale/upscaling_backend.hpp"

#include <d3d11.h>
#include <filesystem>
#include <optional>
#include <wrl/client.h>

namespace neuralfx {
struct UpscaleSources {
    ID3D11Texture2D* color = nullptr;
    ID3D11Texture2D* scene_depth = nullptr;
    ID3D11Buffer* resolve = nullptr;
    temporal::RenderSize render;
    temporal::RenderSize output;
};

class UpscaleInputs {
public:
    HRESULT initialize(ID3D11Device* device, const std::filesystem::path& shader_directory);
    std::optional<upscale::FrameResources> prepare(ID3D11DeviceContext* context, const UpscaleSources& sources) noexcept;
    ID3D11ShaderResourceView* output_view() const noexcept { return output_view_.Get(); }

private:
    bool ensure(ID3D11Device* device, const UpscaleSources& sources) noexcept;
    void build_reactive_mask(ID3D11DeviceContext* context, temporal::RenderSize render) noexcept;

    ResolveFramePass resolve_frame_;
    CameraMotionPass motion_;
    DepthToFloatPass depth_pass_;
    ReactiveMaskPass reactive_pass_;
    MotionTargets targets_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> color_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> color_view_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> previous_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> previous_view_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> reactive_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> reactive_target_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> depth_target_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> output_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> output_view_;
    temporal::RenderSize render_;
    temporal::RenderSize output_size_;
    bool previous_valid_ = false;
};
}
