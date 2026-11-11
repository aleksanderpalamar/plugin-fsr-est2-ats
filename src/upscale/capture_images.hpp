#pragma once

#include "upscale/frame_contract.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace neuralfx::upscale {
struct RgbImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> bytes;
};

struct MotionStats {
    float mean_pixels = 0.0f;
    float max_pixels = 0.0f;
    float fraction_over_eight = 0.0f;
};

float half_to_float(std::uint16_t value) noexcept;
RgbImage rgb_from_rgba8(std::span<const std::uint8_t> texels, std::uint32_t width, std::uint32_t height);
RgbImage depth_image(std::span<const float> depth, std::uint32_t width, std::uint32_t height, const CameraParameters& camera);
RgbImage motion_image(std::span<const std::uint16_t> halves, std::uint32_t width, std::uint32_t height);
MotionStats motion_stats(std::span<const std::uint16_t> halves);
RgbImage grey_image(std::span<const std::uint8_t> values, std::uint32_t width, std::uint32_t height);
float reactive_fraction(std::span<const std::uint8_t> values);
float mean_luma(const RgbImage& image);
}
