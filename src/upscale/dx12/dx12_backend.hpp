#pragma once

#include "upscale/dx12/dx12_context.hpp"
#include "upscale/dx12/ffx_runtime.hpp"
#include "upscale/dx12/shared_texture.hpp"

#include <d3d11_4.h>
#include <filesystem>
#include <memory>

namespace neuralfx::upscale {
inline constexpr DXGI_FORMAT dx12_color_format = DXGI_FORMAT_R8G8B8A8_UNORM;
inline constexpr DXGI_FORMAT dx12_depth_format = DXGI_FORMAT_R32_FLOAT;
inline constexpr DXGI_FORMAT dx12_motion_format = DXGI_FORMAT_R16G16_FLOAT;
inline constexpr DXGI_FORMAT dx12_output_format = DXGI_FORMAT_R8G8B8A8_UNORM;

class Dx12Backend final : public UpscalingBackend {
public:
    Dx12Backend(ID3D11Device* device, ID3D11DeviceContext* context, std::filesystem::path directory) noexcept;

    std::vector<ProviderVersion> providers() override;
    UpscaleStatus open(const ProviderVersion& provider, const OpenParameters& parameters) override;
    std::string provider_name() const override { return name_; }
    UpscaleStatus upscale(const FrameContract& contract, const FrameResources& resources) override;
    void close() override;

private:
    UpscaleStatus prepare() noexcept;
    UpscaleStatus create_textures(const OpenParameters& parameters) noexcept;
    bool accepts(const FrameContract& contract, const FrameResources& resources) const noexcept;
    UpscaleStatus record(const FrameContract& contract) noexcept;

    Microsoft::WRL::ComPtr<ID3D11Device5> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext4> context_;
    Microsoft::WRL::ComPtr<ID3D11Fence> fence_;
    std::filesystem::path directory_;
    std::unique_ptr<Dx12Context> dx12_;
    FfxRuntime runtime_;
    std::optional<SharedTexture> color_;
    std::optional<SharedTexture> depth_;
    std::optional<SharedTexture> motion_;
    std::optional<SharedTexture> output_;
    std::uint64_t fence_value_ = 0;
    std::string name_;
};
}
