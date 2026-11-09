#pragma once

#include "temporal/motion_reprojection.hpp"

#include <optional>

namespace neuralfx::temporal {
enum class TemporalStatus { Ready, FirstFrame, Resized, NoScene, NoCamera, NoJitter };

struct FrameInput {
    std::optional<RenderSize> render;
    std::optional<CameraMatrices> camera;
    bool temporal_resolve = false;
};

struct TemporalDecision {
    TemporalStatus status = TemporalStatus::NoScene;
    std::optional<CameraState> camera;
    std::optional<MotionConstants> motion;
};

class TemporalHistory {
public:
    TemporalDecision advance(const FrameInput& input) noexcept;
    void reset() noexcept;

private:
    std::optional<CameraState> previous_;
    RenderSize size_;
};
}
