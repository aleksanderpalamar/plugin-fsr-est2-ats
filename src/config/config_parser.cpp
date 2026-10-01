#include "config/config.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <istream>
#include <string>
#include <string_view>

namespace neuralfx {
namespace {
std::string_view trim(std::string_view value) noexcept {
    auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

void assign(Config& config, std::string_view key, std::string_view value) noexcept {
    if (key == "enabled") config.enabled = value == "true" || value == "1";
    if (key == "mode" && value == "hook") config.mode = TestMode::Hook;
    if (key == "mode" && value == "copy") config.mode = TestMode::Copy;
    if (key == "mode" && value == "fullscreen") config.mode = TestMode::Fullscreen;
    if (key == "mode" && value == "rcas") config.mode = TestMode::Rcas;
    if (key != "sharpness") return;
    float parsed = 0;
    auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return;
    if (!std::isfinite(parsed)) return;
    config.sharpness = std::clamp(parsed, 0.0f, 1.0f);
}
}

Config parse_config(std::istream& input) {
    Config config;
    std::string line;
    while (std::getline(input, line)) {
        auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        assign(config, trim(std::string_view(line).substr(0, separator)),
            trim(std::string_view(line).substr(separator + 1)));
    }
    return config;
}
}
