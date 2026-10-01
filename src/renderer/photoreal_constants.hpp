#pragma once

#include "config/config.hpp"

#include <cstdint>

namespace neuralfx {
struct alignas(16) PhotorealConstants {
    float rcas_strength;
    float exposure_ev;
    float contrast;
    float saturation;
    float clarity;
    float highlight_boost;
    float highlight_warmth;
    float shadow_coolness;
    float neural_strength;
    float black_level;
    float lut_strength;
    float grain_strength;
    float tone_strength;
    float highlight_start;
    float highlight_end;
    float lut_size;
    std::uint32_t decode_input;
    std::uint32_t srgb_target;
    std::uint32_t has_neural;
    std::uint32_t has_lut;
    float frame_index;
    float padding[3];
};

static_assert(sizeof(PhotorealConstants) == 96);

inline PhotorealConstants make_photoreal_constants(const Config& config,
    bool decode_input, bool srgb_target, bool neural_available,
    unsigned lut_size, float frame_index) noexcept {
    const auto& settings = config.photoreal;
    return {
        settings.photoreal_sharpness,
        settings.exposure_ev,
        settings.contrast,
        settings.saturation,
        settings.clarity,
        settings.highlight_boost,
        settings.highlight_warmth,
        settings.shadow_coolness,
        settings.neural_strength,
        settings.black_level,
        settings.lut_strength,
        settings.grain_strength,
        settings.tone_strength,
        settings.highlight_start,
        settings.highlight_end,
        static_cast<float>(lut_size),
        static_cast<std::uint32_t>(decode_input),
        static_cast<std::uint32_t>(srgb_target),
        static_cast<std::uint32_t>(neural_available && settings.neural_strength > 0.0f),
        static_cast<std::uint32_t>(lut_size > 1 && settings.lut_strength > 0.0f),
        frame_index,
        {0.0f, 0.0f, 0.0f}
    };
}
}
