#pragma once

#include "upscale/frame_contract.hpp"
#include "upscale/upscaling_backend.hpp"

#include <d3d11.h>
#include <filesystem>

namespace neuralfx::upscale {
inline constexpr int capture_burst_frames = 3;
inline constexpr int capture_burst_names = 4;

struct CaptureSources {
    const FrameResources& resources;
    const FrameContract& contract;
    ID3D11Texture2D* scene_depth = nullptr;
    ID3D11Buffer* resolve = nullptr;
};

class FrameCapture {
public:
    explicit FrameCapture(std::filesystem::path directory) : directory_(std::move(directory)) {}
    void request() noexcept;
    bool pending() const noexcept { return remaining_ > 0; }
    bool wants_previous() const noexcept { return remaining_ == capture_burst_frames; }
    void write(ID3D11DeviceContext* context, const CaptureSources& sources) noexcept;

private:
    bool save_frame(ID3D11DeviceContext* context, const CaptureSources& sources, const std::string& stem, bool first);
    void save_extras(ID3D11DeviceContext* context, const CaptureSources& sources, const std::string& stem);

    std::filesystem::path directory_;
    unsigned burst_ = 0;
    int remaining_ = 0;
    int index_ = 0;
};
}
