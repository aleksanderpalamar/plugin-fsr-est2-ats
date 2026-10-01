#pragma once

#include <iosfwd>

namespace neuralfx {
enum class TestMode { Hook, Copy, Fullscreen, Rcas };

struct Config {
    bool enabled = false;
    TestMode mode = TestMode::Hook;
    float sharpness = 0.65f;
};

Config load_config();
Config parse_config(std::istream& input);
}
