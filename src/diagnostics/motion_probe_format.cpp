#include "diagnostics/motion_probe_format.hpp"
#include "diagnostics/number_format.hpp"

namespace neuralfx::diagnostics {
namespace {
using temporal::TemporalStatus;

std::string values(const temporal::Matrix4& matrix) {
    std::string text;
    for (std::size_t index = 0; index < matrix.size(); ++index) text += (index > 0 ? "," : "") + decimal(matrix[index]);
    return text;
}

std::string position(temporal::Position value) {
    return fixed(value.x) + "," + fixed(value.y) + "," + fixed(value.z);
}

const char* fault_name(temporal::CameraFault fault) {
    switch (fault) {
    case temporal::CameraFault::EmptySize: return "empty-size";
    case temporal::CameraFault::SingularView: return "singular-view";
    case temporal::CameraFault::NotPerspective: return "not-perspective";
    case temporal::CameraFault::JitterOutOfRange: return "jitter-out-of-range";
    }
    return "unknown";
}

const char* status_name(TemporalStatus status) {
    switch (status) {
    case TemporalStatus::Ready: return "ready";
    case TemporalStatus::FirstFrame: return "first-frame";
    case TemporalStatus::Resized: return "resized";
    case TemporalStatus::CameraCut: return "camera-cut";
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

namespace neuralfx::diagnostics {
std::string rejection_line(const temporal::RejectedCamera& rejected) {
    const auto& rejection = rejected.rejection;
    std::string line = "Motion camera rejected frame=" + std::to_string(rejected.frame) + " reason=" +
        fault_name(rejection.fault);
    if (rejection.fault == temporal::CameraFault::SingularView) return line;
    const auto& p = rejection.projection;
    return line + " row3=" + decimal(p[12]) + "," + decimal(p[13]) + "," + decimal(p[14]) + "," + decimal(p[15]) +
        " offsets=" + decimal(p[2]) + "," + decimal(p[6]);
}

std::string jump_line(const temporal::CameraStep& step) {
    return "Motion camera jump frame=" + std::to_string(step.frame) + " move_m=" + fixed(step.distance_m) +
        " turn_deg=" + fixed(step.angle_deg) + " from=" + position(step.from) + " to=" + position(step.to);
}

std::string unmatched_line(const temporal::UnmatchedCamera& camera) {
    return "Motion camera unmatched frame=" + std::to_string(camera.frame) + " jitter=" + decimal(camera.jitter.x) +
        "," + decimal(camera.jitter.y) + " view=" + values(camera.matrices.view) +
        " view_projection=" + values(camera.matrices.view_projection);
}
}
