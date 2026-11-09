#pragma once

#include "diagnostics/frame_trace.hpp"
#include "diagnostics/pipeline_state.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace neuralfx::diagnostics {
enum class SessionState { Idle, Armed, Recording, Paused };

class TraceSession {
public:
    explicit TraceSession(std::size_t entry_limit) noexcept : trace_(entry_limit) {}

    bool request(std::uint32_t frames) noexcept;
    std::vector<std::string> present_begin();
    void present_end(const TraceTarget& backbuffer);
    SessionState state() const noexcept { return state_; }
    bool recording() const noexcept { return state_ == SessionState::Recording; }
    FrameTrace& trace() noexcept { return trace_; }
    PipelineState& pipeline() noexcept { return pipeline_; }

private:
    FrameTrace trace_;
    PipelineState pipeline_;
    SessionState state_ = SessionState::Idle;
    std::uint32_t remaining_ = 0;
};
}
