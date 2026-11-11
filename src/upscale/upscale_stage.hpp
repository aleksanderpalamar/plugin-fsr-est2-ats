#pragma once

#include "common/logger.hpp"
#include "diagnostics/trace_trigger.hpp"
#include "renderer/blit_pass.hpp"
#include "renderer/upscale_inputs.hpp"
#include "temporal/jitter_tracker.hpp"
#include "temporal/scene_tracker.hpp"
#include "upscale/frame_capture.hpp"
#include "upscale/motion_trigger.hpp"
#include "upscale/resolve_replacement.hpp"
#include "upscale/upscale_counters.hpp"
#include "upscale/upscale_session.hpp"
#include "temporal/camera_continuity.hpp"

#include <chrono>
#include <filesystem>
#include <string>

namespace neuralfx::upscale {
class UpscaleStage final : public temporal::PassListener {
public:
    UpscaleStage(UpscalingBackend& backend, UpscaleMode mode) noexcept : session_(backend, mode) {}
    HRESULT initialize(ID3D11Device* device, const std::filesystem::path& shader_directory);
    temporal::DrawDecision first_draw(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept override;
    void begin_frame() noexcept;
    void observe(const temporal::SceneSample& sample) noexcept;
    const SessionReport& report() const noexcept { return session_.report(); }
    const UpscaleCounters& counters() const noexcept { return counters_; }

private:
    temporal::DrawDecision on_resolve(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept;
    temporal::DrawDecision on_output(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept;
    bool refuse_oversized(temporal::RenderSize render, temporal::RenderSize output) noexcept;
    bool ensure_session(temporal::RenderSize render, temporal::RenderSize output) noexcept;
    FrameContract contract_for(const temporal::PassEvent& event, temporal::RenderSize output, temporal::Jitter jitter) noexcept;
    void note(const std::string& message) noexcept;
    void report_failure(UpscaleStatus status, const FrameContract& contract) noexcept;

    UpscaleSession session_;
    MotionTrigger capture_trigger_;
    FrameCapture capture_{module_directory() / L"NeuralFX" / L"captures"};
    diagnostics::TraceTrigger capture_request_{module_directory() / L"NeuralFX" / L"fsr-capture"};
    UpscaleInputs inputs_;
    BlitPass blit_;
    ResolveReplacement replacement_;
    temporal::JitterTracker tracker_;
    temporal::RenderSize opened_render_;
    temporal::RenderSize opened_output_;
    std::chrono::steady_clock::time_point last_output_;
    float vertical_fov_ = 0.0f;
    bool opened_ = false;
    bool replaced_ = false;
    bool pending_reset_ = true;
    bool ready_ = false;
    std::uint64_t notes_ = 0;
    UpscaleCounters counters_;
};
}
