#pragma once

#include "upscale/upscaling_backend.hpp"

#include <string>

namespace neuralfx::upscale {
enum class SessionState { Off, Inactive, Active, Failed };

struct SessionReport {
    SessionState state = SessionState::Inactive;
    SelectionReason reason = SelectionReason::Off;
    std::string active_version;
};

class UpscaleSession {
public:
    UpscaleSession(UpscalingBackend& backend, UpscaleMode mode) noexcept : backend_(backend), mode_(mode) {}
    UpscaleStatus open(temporal::RenderSize max_render, temporal::RenderSize max_output);
    UpscaleStatus process(const FrameContract& contract, const FrameResources& resources);
    void close() noexcept;
    const SessionReport& report() const noexcept { return report_; }

private:
    UpscaleStatus fail(UpscaleStatus status) noexcept;

    UpscalingBackend& backend_;
    UpscaleMode mode_;
    SessionReport report_;
    bool pending_reset_ = false;
};
}
