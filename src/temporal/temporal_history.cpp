#include "temporal/temporal_history.hpp"

namespace neuralfx::temporal {
namespace {
bool same_size(RenderSize left, RenderSize right) {
    return left.width == right.width && left.height == right.height;
}
}

TemporalDecision TemporalHistory::advance(const FrameInput& input) noexcept {
    if (!input.render) {
        reset();
        return {TemporalStatus::NoScene, std::nullopt, std::nullopt};
    }
    auto camera = input.camera ? camera_state(*input.camera, *input.render) : std::nullopt;
    if (!camera) {
        reset();
        return {TemporalStatus::NoCamera, std::nullopt, std::nullopt};
    }
    if (!input.temporal_resolve) {
        reset();
        return {TemporalStatus::NoJitter, camera, std::nullopt};
    }
    auto previous = previous_;
    bool resized = previous && !same_size(size_, *input.render);
    previous_ = camera;
    size_ = *input.render;
    if (!previous) return {TemporalStatus::FirstFrame, camera, std::nullopt};
    if (resized) return {TemporalStatus::Resized, camera, std::nullopt};
    auto motion = motion_constants(*previous, *camera, *input.render);
    if (!motion) {
        reset();
        return {TemporalStatus::NoCamera, camera, std::nullopt};
    }
    return {TemporalStatus::Ready, camera, motion};
}

void TemporalHistory::reset() noexcept {
    previous_.reset();
    size_ = {};
}
}
