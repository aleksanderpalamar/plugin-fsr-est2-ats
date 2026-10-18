#pragma once

#include "raytracing/gpu_constants.hpp"
#include "raytracing/gpu_scene_buffers.hpp"
#include "raytracing/image.hpp"

#include <d3d11.h>
#include <filesystem>
#include <optional>
#include <wrl/client.h>

namespace neuralfx::raytracing {
class GpuRaytracer {
public:
    HRESULT initialize(ID3D11Device* device, const std::filesystem::path& shader_directory);
    HRESULT upload(const GpuSceneData& scene);
    std::optional<Image> render(ID3D11DeviceContext* context, const Camera& camera,
        int width, int height, const Settings& settings);
    void reset() noexcept;

private:
    HRESULT prepare_output(UINT width, UINT height);
    void dispatch(ID3D11DeviceContext* context, UINT width, UINT height) noexcept;

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> output_;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> output_view_;
    GpuSceneBuffers scene_;
    UINT output_width_ = 0;
    UINT output_height_ = 0;
};
}
