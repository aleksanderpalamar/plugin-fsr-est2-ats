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
}

template <typename Action>
void record(Action&& action) noexcept {
    if (!detail::tracing().load(std::memory_order_acquire)) return;
    std::lock_guard lock(detail::trace_mutex());
    if (!detail::trace_session().recording()) return;
    try {
        action(detail::trace_session().trace());
    } catch (...) {
    }
}
}
