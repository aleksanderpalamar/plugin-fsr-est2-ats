#include "renderer/capture_policy.hpp"

int main() {
    int primary = 0;
    int secondary = 0;
    if (!neuralfx::resets_active_capture(&primary, &primary)) return 1;
    if (neuralfx::resets_active_capture(&primary, &secondary)) return 2;
    int* absent = nullptr;
    if (!neuralfx::resets_active_capture(&primary, absent)) return 7;

    neuralfx::Config config;
    config.mode = neuralfx::TestMode::Raytracing;
    if (neuralfx::captures_depth(true, config)) return 3;
    config.enabled = true;
    if (!neuralfx::captures_depth(true, config)) return 4;
    if (neuralfx::captures_depth(false, config)) return 5;
    config.mode = neuralfx::TestMode::Photoreal;
    if (neuralfx::captures_depth(true, config)) return 6;
    return 0;
}
