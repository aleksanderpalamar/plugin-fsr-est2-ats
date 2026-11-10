#include "upscale/upscale_session.hpp"

namespace neuralfx::upscale {
namespace {
bool complete(const FrameResources& resources) {
    return resources.color && resources.depth && resources.motion && resources.output;
}

bool same_version(const std::string& selected, const std::string& active) {
    auto left = parse_version(selected);
    return left && left == parse_version(active);
}
}

UpscaleStatus UpscaleSession::open(temporal::RenderSize max_render, temporal::RenderSize max_output) {
    report_ = {};
    pending_reset_ = false;
    if (mode_ == UpscaleMode::Off) {
        report_.state = SessionState::Off;
        return UpscaleStatus::Unsupported;
    }
    auto selection = select_provider(mode_, backend_.providers());
    report_.reason = selection.reason;
    if (!selection.provider) return UpscaleStatus::Unsupported;
    auto status = backend_.open(*selection.provider, max_render, max_output);
    if (status != UpscaleStatus::Ready) return fail(status);
    auto active = backend_.provider_name();
    if (!same_version(selection.provider->name, active)) return fail(UpscaleStatus::BackendFailure);
    report_.state = SessionState::Active;
    report_.active_version = active;
    return UpscaleStatus::Ready;
}

UpscaleStatus UpscaleSession::process(const FrameContract& contract, const FrameResources& resources) {
    if (report_.state != SessionState::Active) return UpscaleStatus::Unsupported;
    auto status = validate(contract);
    if (status == UpscaleStatus::Ready && !complete(resources)) status = UpscaleStatus::InvalidInput;
    if (status != UpscaleStatus::Ready) {
        pending_reset_ = true;
        return status;
    }
    FrameContract frame = contract;
    frame.reset = frame.reset || pending_reset_;
    status = backend_.upscale(frame, resources);
    if (status == UpscaleStatus::Ready) pending_reset_ = false;
    if (status == UpscaleStatus::InvalidInput) pending_reset_ = true;
    if (status == UpscaleStatus::Ready || status == UpscaleStatus::InvalidInput) return status;
    return fail(status);
}

void UpscaleSession::close() noexcept {
    if (report_.state != SessionState::Active) return;
    backend_.close();
    report_.state = SessionState::Inactive;
    report_.active_version.clear();
}

UpscaleStatus UpscaleSession::fail(UpscaleStatus status) noexcept {
    backend_.close();
    report_.state = SessionState::Failed;
    report_.active_version.clear();
    return status;
}
}
