#include "config/config.hpp"

#include <sstream>

int main() {
    std::istringstream empty;
    auto defaults = neuralfx::parse_config(empty);
    if (defaults.enabled || defaults.mode != neuralfx::TestMode::Hook) return 1;
    std::istringstream valid(" enabled = true\nmode=rcas\nsharpness=0.8\n");
    auto configured = neuralfx::parse_config(valid);
    if (!configured.enabled || configured.mode != neuralfx::TestMode::Rcas) return 2;
    if (configured.sharpness < 0.79f || configured.sharpness > 0.81f) return 3;
    std::istringstream invalid("sharpness=nope\nmode=unknown\n");
    auto fallback = neuralfx::parse_config(invalid);
    if (fallback.sharpness != 0.65f || fallback.mode != neuralfx::TestMode::Hook) return 4;
    std::istringstream clamped("sharpness=2.5\n");
    if (neuralfx::parse_config(clamped).sharpness != 1.0f) return 5;
    return 0;
}
