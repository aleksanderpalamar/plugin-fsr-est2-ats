#pragma once

namespace neuralfx::upscale {
inline constexpr float motion_trigger_threshold_m = 0.1f;
inline constexpr int motion_trigger_sustained_frames = 90;
inline constexpr int motion_trigger_cooldown_frames = 1800;
inline constexpr int motion_trigger_max_bursts = 3;

class MotionTrigger {
public:
    bool observe(float move_m) noexcept;

private:
    int moving_ = 0;
    int cooldown_ = 0;
    int fired_ = 0;
};
}
