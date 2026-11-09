#include "diagnostics/trace_session.hpp"

namespace neuralfx::diagnostics {
bool TraceSession::request(std::uint32_t frames) noexcept {
    if (frames == 0 || state_ != SessionState::Idle) return false;
    remaining_ = frames;
    state_ = SessionState::Armed;
    return true;
}

std::vector<std::string> TraceSession::present_begin() {
    if (state_ != SessionState::Recording) return {};
    --remaining_;
    state_ = remaining_ == 0 ? SessionState::Idle : SessionState::Paused;
    return trace_.lines();
}

void TraceSession::present_end(const TraceTarget& backbuffer) {
    if (state_ != SessionState::Armed && state_ != SessionState::Paused) return;
    trace_.begin(backbuffer);
    state_ = SessionState::Recording;
}
}
