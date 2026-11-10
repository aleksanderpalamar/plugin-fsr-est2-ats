#pragma once

#include "upscale/upscaling_backend.hpp"

#include <d3d12.h>
#include <filesystem>
#include <string>
#include <vector>
#include <windows.h>

namespace neuralfx::upscale {
struct FfxInputs {
    ID3D12GraphicsCommandList* list = nullptr;
    ID3D12Resource* color = nullptr;
    ID3D12Resource* depth = nullptr;
    ID3D12Resource* motion = nullptr;
    ID3D12Resource* output = nullptr;
};

class FfxRuntime {
public:
    FfxRuntime() = default;
    ~FfxRuntime();
    FfxRuntime(const FfxRuntime&) = delete;
    FfxRuntime& operator=(const FfxRuntime&) = delete;

    UpscaleStatus load(const std::filesystem::path& directory) noexcept;
    std::vector<ProviderVersion> versions(ID3D12Device* device);
    UpscaleStatus create(ID3D12Device* device, const ProviderVersion& provider, const OpenParameters& parameters) noexcept;
    std::string provider_name() noexcept;
    UpscaleStatus dispatch(const FfxInputs& inputs, const FrameContract& contract) noexcept;
    void destroy() noexcept;

private:
    struct Api;
    HMODULE module_ = nullptr;
    Api* api_ = nullptr;
    void* context_ = nullptr;
};
}
