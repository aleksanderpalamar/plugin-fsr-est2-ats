#include "config/config.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <istream>
#include <optional>
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

std::optional<float> parse_number(std::string_view value) noexcept {
    float parsed = 0;
    auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return std::nullopt;
    if (!std::isfinite(parsed)) return std::nullopt;
    return parsed;
}

struct Parameter {
    std::string_view name;
    float PhotorealSettings::* member;
    float minimum;
    float maximum;
};

constexpr std::array parameters{
    Parameter{"exposure_ev", &PhotorealSettings::exposure_ev, -2.0f, 2.0f},
    Parameter{"contrast", &PhotorealSettings::contrast, 0.0f, 2.0f},
    Parameter{"saturation", &PhotorealSettings::saturation, 0.0f, 2.0f},
    Parameter{"clarity", &PhotorealSettings::clarity, 0.0f, 1.0f},
    Parameter{"highlight_boost", &PhotorealSettings::highlight_boost, 0.0f, 1.0f},
    Parameter{"highlight_warmth", &PhotorealSettings::highlight_warmth, 0.0f, 1.0f},
    Parameter{"shadow_coolness", &PhotorealSettings::shadow_coolness, 0.0f, 1.0f},
    Parameter{"neural_strength", &PhotorealSettings::neural_strength, 0.0f, 1.0f},
    Parameter{"black_level", &PhotorealSettings::black_level, 0.0f, 0.01f},
    Parameter{"lut_strength", &PhotorealSettings::lut_strength, 0.0f, 1.0f},
    Parameter{"grain_strength", &PhotorealSettings::grain_strength, 0.0f, 0.02f},
    Parameter{"photoreal_sharpness", &PhotorealSettings::photoreal_sharpness, 0.0f, 1.0f},
    Parameter{"tone_strength", &PhotorealSettings::tone_strength, 0.0f, 1.0f},
    Parameter{"highlight_start", &PhotorealSettings::highlight_start, 0.0f, 1.0f},
    Parameter{"highlight_end", &PhotorealSettings::highlight_end, 0.0f, 1.0f}
};

void assign(Config& config, std::string_view key, std::string_view value) {
    if (key == "enabled") config.enabled = value == "true" || value == "1";
    if (key == "mode" && value == "hook") config.mode = TestMode::Hook;
    if (key == "mode" && value == "copy") config.mode = TestMode::Copy;
    if (key == "mode" && value == "fullscreen") config.mode = TestMode::Fullscreen;
    if (key == "mode" && value == "rcas") config.mode = TestMode::Rcas;
    if (key == "mode" && value == "photoreal") config.mode = TestMode::Photoreal;
    if (key == "lut_path") config.lut_path = value;
    auto parsed = parse_number(value);
    if (!parsed) return;
    if (key == "sharpness") config.sharpness = std::clamp(*parsed, 0.0f, 1.0f);
    for (const auto& parameter : parameters) {
        if (key != parameter.name) continue;
        config.photoreal.*(parameter.member) = std::clamp(*parsed, parameter.minimum, parameter.maximum);
        return;
    }
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
    config.photoreal.highlight_end = std::max(0.01f, config.photoreal.highlight_end);
    if (config.photoreal.highlight_start >= config.photoreal.highlight_end)
        config.photoreal.highlight_start = config.photoreal.highlight_end - 0.01f;
    return config;
}
}
