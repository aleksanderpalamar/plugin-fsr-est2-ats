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
    }
    return "unknown";
}
}

std::string motion_line(std::uint64_t frame, const temporal::TemporalDecision& decision,
    std::optional<temporal::RenderSize> render, std::int64_t wait_us) {
    std::string line = "Motion frame " + std::to_string(frame) + " status=" + status_name(decision.status);
    if (render) line += " render=" + std::to_string(render->width) + "x" + std::to_string(render->height);
    if (decision.camera)
        line += " jitter=" + decimal(decision.camera->jitter.x) + "," + decimal(decision.camera->jitter.y);
    return line + " wait_us=" + std::to_string(wait_us);
}
}
