#pragma once

#include "upscale/frame_contract.hpp"
#include "upscale/provider_policy.hpp"

#include <string>
#include <vector>

struct ID3D11Texture2D;

namespace neuralfx::upscale {
struct FrameResources {
    ID3D11Texture2D* color = nullptr;
    ID3D11Texture2D* depth = nullptr;
    ID3D11Texture2D* motion = nullptr;
    ID3D11Texture2D* output = nullptr;
    ID3D11Texture2D* reactive = nullptr;
};

struct OpenParameters {
    temporal::RenderSize max_render;
    temporal::RenderSize max_output;
    bool depth_inverted = true;
    bool color_non_linear = true;
};

class UpscalingBackend {
public:
    virtual ~UpscalingBackend() = default;
    virtual std::vector<ProviderVersion> providers() = 0;
    virtual UpscaleStatus open(const ProviderVersion& provider, const OpenParameters& parameters) = 0;
    virtual std::string provider_name() const = 0;
    virtual UpscaleStatus upscale(const FrameContract& contract, const FrameResources& resources) = 0;
    virtual void close() = 0;
};
}
