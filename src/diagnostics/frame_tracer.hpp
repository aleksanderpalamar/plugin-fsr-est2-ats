#pragma once

#include "diagnostics/trace_session.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace neuralfx::diagnostics {
bool request_capture(std::uint32_t frames) noexcept;
void poll_capture_request() noexcept;
std::vector<std::string> trace_present_begin() noexcept;
void trace_present_end(IDXGISwapChain* swapchain) noexcept;
void trace_targets_bound(ID3D11DeviceContext* context) noexcept;

namespace detail {
std::atomic_bool& tracing() noexcept;
std::mutex& trace_mutex() noexcept;
TraceSession& trace_session() noexcept;

inline bool& recording_thread() noexcept {
    thread_local bool active = false;
    return active;
}

struct RecordingScope {
    RecordingScope() noexcept { recording_thread() = true; }
    ~RecordingScope() { recording_thread() = false; }
    RecordingScope(const RecordingScope&) = delete;
    RecordingScope& operator=(const RecordingScope&) = delete;
};
}

template <typename Action>
void record_pipeline(Action&& action) noexcept {
    if (!detail::tracing().load(std::memory_order_acquire) || detail::recording_thread()) return;
    std::lock_guard lock(detail::trace_mutex());
    detail::RecordingScope scope;
    auto& session = detail::trace_session();
    if (!session.recording()) return;
    try {
        action(session.trace(), session.pipeline());
    } catch (...) {
    }
}

template <typename Action>
void record(Action&& action) noexcept {
    record_pipeline([&](FrameTrace& trace, PipelineState&) { action(trace); });
}
}
