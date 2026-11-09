#include "diagnostics/motion_probe_format.hpp"

#include <array>
#include <charconv>

namespace neuralfx::diagnostics {
namespace {
using temporal::TemporalStatus;

std::string decimal(float value) {
    std::array<char, 32> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    return std::string(buffer.data(), result.ptr);
}

const char* status_name(TemporalStatus status) {
    switch (status) {
    case TemporalStatus::Ready: return "ready";
    case TemporalStatus::FirstFrame: return "first-frame";
    case TemporalStatus::Resized: return "resized";
    case TemporalStatus::NoScene: return "no-scene";
    case TemporalStatus::NoCamera: return "no-camera";
    case TemporalStatus::NoJitter: return "no-jitter";
    case TemporalStatus::Unsynced: return "unsynced";
    }
    return "unknown";
}

const char* sync_name(temporal::SyncResult result) {
    switch (result) {
    case temporal::SyncResult::Confirmed: return "confirmed";
    case temporal::SyncResult::Resynced: return "resynced";
    case temporal::SyncResult::Lost: return "lost";
    }
    return "unknown";
}
}

std::string motion_line(std::uint64_t frame, const temporal::TemporalDecision& decision,
    std::optional<temporal::RenderSize> render, std::int64_t cpu_us) {
    std::string line = "Motion frame " + std::to_string(frame) + " status=" + status_name(decision.status);
    if (render) line += " render=" + std::to_string(render->width) + "x" + std::to_string(render->height);
    if (decision.jitter) line += " jitter=" + decimal(decision.jitter->x) + "," + decimal(decision.jitter->y);
    if (decision.sync)
        line += std::string(" sync=") + sync_name(decision.sync->result) + "@" + std::to_string(decision.sync->frame);
    return line + " cpu_us=" + std::to_string(cpu_us);
}
}
