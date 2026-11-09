#include "diagnostics/frame_tracer.hpp"
#include "diagnostics/trace_hooks.hpp"
#include "diagnostics/trace_resources.hpp"
#include "diagnostics/trace_trigger.hpp"
#include "common/logger.hpp"

#include <array>
#include <wrl/client.h>

namespace neuralfx::diagnostics {
namespace {
constexpr std::uint32_t requested_frames = 2;

std::optional<TraceTarget> backbuffer_of(IDXGISwapChain* swapchain) noexcept {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backbuffer;
    if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)))) return std::nullopt;
    return describe_resource(backbuffer.Get());
}

bool hook_device(IDXGISwapChain* swapchain) noexcept {
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    if (FAILED(swapchain->GetDevice(IID_PPV_ARGS(&device)))) return false;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(&context);
    return install_trace_hooks(context.Get());
}

void bind_current(FrameTrace& trace, ID3D11DeviceContext* context) {
    std::array<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> views;
    std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> raw{};
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth;
    context->OMGetRenderTargets(static_cast<UINT>(raw.size()), raw.data(), &depth);
    std::vector<TraceTarget> colors;
    for (size_t index = 0; index < raw.size(); ++index) {
        views[index].Attach(raw[index]);
        if (auto target = describe_view(raw[index])) colors.push_back(*target);
    }
    trace.bind(colors, describe_view(depth.Get()));
}
}

bool request_capture(std::uint32_t frames) noexcept {
    std::lock_guard lock(detail::trace_mutex());
    return detail::trace_session().request(frames);
}

void poll_capture_request() noexcept {
    try {
        static TraceTrigger trigger(module_directory() / L"NeuralFX" / L"capture-trace");
        auto result = trigger.poll(std::chrono::steady_clock::now());
        if (result == TriggerResult::Failed) log("Trace capture request could not be removed");
        if (result != TriggerResult::Requested) return;
        log(request_capture(requested_frames) ? "Trace capture requested" : "Trace capture already running");
    } catch (...) {
        log("Trace capture request failed");
    }
}

std::vector<std::string> trace_present_begin() noexcept {
    std::lock_guard lock(detail::trace_mutex());
    detail::tracing().store(false, std::memory_order_release);
    try {
        return detail::trace_session().present_begin();
    } catch (...) {
        return {"Trace frame lost: out of memory"};
    }
}

void trace_present_end(IDXGISwapChain* swapchain) noexcept {
    std::lock_guard lock(detail::trace_mutex());
    auto& session = detail::trace_session();
    if (!swapchain || session.state() == SessionState::Idle) return;
    if (!hook_device(swapchain)) log("Trace draw hooks unavailable");
    auto backbuffer = backbuffer_of(swapchain);
    if (!backbuffer) return;
    try {
        session.present_end(*backbuffer);
    } catch (...) {
        return;
    }
    detail::tracing().store(session.recording(), std::memory_order_release);
}

void trace_targets_bound(ID3D11DeviceContext* context) noexcept {
    record([&](FrameTrace& trace) { bind_current(trace, context); });
}

namespace detail {
std::atomic_bool& tracing() noexcept {
    static std::atomic_bool active{false};
    return active;
}

std::mutex& trace_mutex() noexcept {
    static std::mutex mutex;
    return mutex;
}

TraceSession& trace_session() noexcept {
    static TraceSession session(4096);
    return session;
}
}
}
