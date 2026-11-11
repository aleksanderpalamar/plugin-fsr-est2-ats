#include "upscale/motion_trigger.hpp"

namespace neuralfx::upscale {
bool MotionTrigger::observe(float move_m) noexcept {
    if (cooldown_ > 0) --cooldown_;
    moving_ = move_m >= motion_trigger_threshold_m ? moving_ + 1 : 0;
    if (moving_ < motion_trigger_sustained_frames || cooldown_ > 0 || fired_ >= motion_trigger_max_bursts) return false;
    ++fired_;
    cooldown_ = motion_trigger_cooldown_frames;
    moving_ = 0;
    return true;
}
}
