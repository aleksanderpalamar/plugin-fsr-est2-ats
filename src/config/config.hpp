#pragma once

#include <iosfwd>
#include <string>

namespace neuralfx {
enum class TestMode { Hook, Copy, Fullscreen, Rcas, Photoreal, Raytracing };

struct PhotorealSettings {
    float exposure_ev = 0.10f;
    float contrast = 0.96f;
    float saturation = 0.84f;
    float clarity = 0.18f;
    float highlight_boost = 0.10f;
    float highlight_warmth = 0.18f;
    float shadow_coolness = 0.08f;
    float neural_strength = 0.0f;
    float black_level = 0.003f;
    float lut_strength = 0.0f;
    float grain_strength = 0.001f;
    float photoreal_sharpness = 0.30f;
    float tone_strength = 0.20f;
    float highlight_start = 0.45f;
    float highlight_end = 0.90f;
};

struct Config {
    bool enabled = false;
    TestMode mode = TestMode::Hook;
    float sharpness = 0.65f;
    PhotorealSettings photoreal;
    std::string lut_path;
};

Config load_config();
Config parse_config(std::istream& input);
}
