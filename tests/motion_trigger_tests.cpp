#include "upscale/motion_trigger.hpp"

#include <iostream>

using namespace neuralfx::upscale;

namespace {
int fire_after_sustained_motion() {
    MotionTrigger trigger;
    for (int frame = 0; frame < motion_trigger_sustained_frames - 1; ++frame)
        if (trigger.observe(0.2f)) return 10;
    return trigger.observe(0.2f) ? 0 : 11;
}

int stopping_resets_the_count() {
    MotionTrigger trigger;
    for (int frame = 0; frame < motion_trigger_sustained_frames - 5; ++frame) trigger.observe(0.2f);
    trigger.observe(0.0f);
    for (int frame = 0; frame < motion_trigger_sustained_frames - 1; ++frame)
        if (trigger.observe(0.2f)) return 20;
    return 0;
}

int cooldown_and_limit() {
    MotionTrigger trigger;
    int fired = 0;
    for (int frame = 0; frame < motion_trigger_cooldown_frames * (motion_trigger_max_bursts + 2); ++frame)
        if (trigger.observe(0.5f)) ++fired;
    return fired == motion_trigger_max_bursts ? 0 : 30;
}

int slow_motion_never_fires() {
    MotionTrigger trigger;
    for (int frame = 0; frame < motion_trigger_sustained_frames * 4; ++frame)
        if (trigger.observe(0.05f)) return 40;
    return 0;
}
}

int main() {
    for (auto test_case : {fire_after_sustained_motion, stopping_resets_the_count, cooldown_and_limit, slow_motion_never_fires}) {
        if (int code = test_case(); code != 0) {
            std::cerr << "motion_trigger case failed: " << code << '\n';
            return code;
        }
    }
    return 0;
}
