#pragma once

#include "upscale/frame_contract.hpp"
#include "upscale/upscaling_backend.hpp"

#include <d3d11.h>
#include <filesystem>

namespace neuralfx::upscale {
class FrameCapture {
public:
    explicit FrameCapture(std::filesystem::path directory) : directory_(std::move(directory)) {}
    void request() noexcept { pending_ = true; }
    bool pending() const noexcept { return pending_; }
    void write(ID3D11DeviceContext* context, const FrameResources& resources, const FrameContract& contract) noexcept;

private:
    bool save_all(ID3D11DeviceContext* context, const FrameResources& resources, const FrameContract& contract);

    std::filesystem::path directory_;
    unsigned sequence_ = 0;
    bool pending_ = false;
};
}
