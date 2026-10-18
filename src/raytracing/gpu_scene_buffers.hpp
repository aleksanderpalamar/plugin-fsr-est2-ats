#pragma once

#include "raytracing/gpu_scene.hpp"

#include <d3d11.h>
#include <array>
#include <wrl/client.h>

namespace neuralfx::raytracing {
class GpuSceneBuffers {
public:
    static constexpr size_t view_count = 4;

    HRESULT upload(ID3D11Device* device, const GpuSceneData& scene);
    void reset() noexcept;
    bool ready() const noexcept { return ready_; }
    GpuSceneCounts counts() const noexcept { return counts_; }
    std::array<ID3D11ShaderResourceView*, view_count> views() const noexcept;

private:
    std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, view_count> buffers_;
    std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, view_count> views_;
    GpuSceneCounts counts_;
    bool ready_ = false;
};
}
