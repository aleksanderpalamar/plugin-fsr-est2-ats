#include "config/config.hpp"

#include <iostream>
#include <sstream>

using neuralfx::upscale::UpscaleMode;

namespace {
UpscaleMode mode_of(const char* text) {
    std::istringstream input(text);
    return neuralfx::parse_config(input).upscale_mode;
}

int defaults_case() {
    std::istringstream empty;
    return neuralfx::parse_config(empty).upscale_mode == UpscaleMode::Off ? 0 : 10;
}

int values_case() {
    if (mode_of("upscale_mode=off\n") != UpscaleMode::Off) return 20;
    if (mode_of("upscale_mode=strict\n") != UpscaleMode::Strict) return 21;
    if (mode_of("upscale_mode=auto\n") != UpscaleMode::Auto) return 22;
    if (mode_of(" upscale_mode = auto \r\n") != UpscaleMode::Auto) return 23;
    return mode_of("enabled=true\nupscale_mode=strict\nsharpness=0.5\n") == UpscaleMode::Strict ? 0 : 24;
}

int invalid_case() {
    for (const char* text : {"upscale_mode=fsr4\n", "upscale_mode=\n", "upscale_mode=AUTO\n", "upscale_mode=1\n"})
        if (mode_of(text) != UpscaleMode::Off) return 30;
    return mode_of("upscale_mode=strict\nupscale_mode=bogus\n") == UpscaleMode::Strict ? 0 : 31;
}

int live_case() {
    neuralfx::Config active;
    neuralfx::Config reloaded;
    active.upscale_mode = UpscaleMode::Auto;
    reloaded.upscale_mode = UpscaleMode::Strict;
    neuralfx::refresh_live_settings(active, reloaded);
    return active.upscale_mode == UpscaleMode::Auto ? 0 : 40;
}
}

int main() {
    for (auto test_case : {defaults_case, values_case, invalid_case, live_case}) {
        if (int code = test_case(); code != 0) {
            std::cerr << "upscale_config case failed: " << code << '\n';
            return code;
        }
    }
    return 0;
}
