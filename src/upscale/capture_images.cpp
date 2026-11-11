#include "upscale/capture_images.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace neuralfx::upscale {
namespace {
constexpr float levels_per_pixel = 16.0f;

std::uint8_t level(float value) {
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

float view_distance(float depth, const CameraParameters& camera) {
    float clamped = std::clamp(depth, 0.0f, 1.0f);
    return camera.near_plane * camera.far_plane / (clamped * (camera.far_plane - camera.near_plane) + camera.near_plane);
}

void push_grey(std::vector<std::uint8_t>& bytes, std::uint8_t value) {
    bytes.insert(bytes.end(), {value, value, value});
}
}

float half_to_float(std::uint16_t value) noexcept {
    int exponent = (value >> 10) & 31;
    int mantissa = value & 1023;
    float sign = (value >> 15) ? -1.0f : 1.0f;
    if (exponent == 0) return sign * std::ldexp(static_cast<float>(mantissa), -24);
    if (exponent == 31) return sign * INFINITY;
    return sign * std::ldexp(static_cast<float>(mantissa + 1024), exponent - 25);
}

RgbImage rgb_from_rgba8(std::span<const std::uint8_t> texels, std::uint32_t width, std::uint32_t height) {
    RgbImage image{width, height, {}};
    image.bytes.reserve(static_cast<std::size_t>(width) * height * 3);
    for (std::size_t index = 0; index + 3 < texels.size(); index += 4)
        image.bytes.insert(image.bytes.end(), {texels[index], texels[index + 1], texels[index + 2]});
    return image;
}

RgbImage depth_image(std::span<const float> depth, std::uint32_t width, std::uint32_t height, const CameraParameters& camera) {
    RgbImage image{width, height, {}};
    image.bytes.reserve(depth.size() * 3);
    float span = std::log(camera.far_plane / camera.near_plane);
    for (float value : depth)
        push_grey(image.bytes, level(1.0f - std::log(view_distance(value, camera) / camera.near_plane) / span));
    return image;
}

RgbImage motion_image(std::span<const std::uint16_t> halves, std::uint32_t width, std::uint32_t height) {
    RgbImage image{width, height, {}};
    image.bytes.reserve(halves.size() / 2 * 3);
    for (std::size_t index = 0; index + 1 < halves.size(); index += 2) {
        float x = half_to_float(halves[index]);
        float y = half_to_float(halves[index + 1]);
        image.bytes.insert(image.bytes.end(),
            {level(0.5f + x * levels_per_pixel / 255.0f), level(0.5f + y * levels_per_pixel / 255.0f), 128});
    }
    return image;
}

MotionStats motion_stats(std::span<const std::uint16_t> halves) {
    MotionStats stats;
    std::size_t count = halves.size() / 2;
    std::size_t over = 0;
    double total = 0.0;
    for (std::size_t index = 0; index < count; ++index) {
        float x = half_to_float(halves[index * 2]);
        float y = half_to_float(halves[index * 2 + 1]);
        float length = std::isfinite(x) && std::isfinite(y) ? std::sqrt(x * x + y * y) : 0.0f;
        total += length;
        stats.max_pixels = std::max(stats.max_pixels, length);
        if (length > 8.0f) ++over;
    }
    if (count == 0) return stats;
    stats.mean_pixels = static_cast<float>(total / static_cast<double>(count));
    stats.fraction_over_eight = static_cast<float>(over) / static_cast<float>(count);
    return stats;
}

RgbImage grey_image(std::span<const std::uint8_t> values, std::uint32_t width, std::uint32_t height) {
    RgbImage image{width, height, {}};
    image.bytes.reserve(values.size() * 3);
    for (std::uint8_t value : values) push_grey(image.bytes, value);
    return image;
}

float reactive_fraction(std::span<const std::uint8_t> values) {
    if (values.empty()) return 0.0f;
    auto marked = std::count_if(values.begin(), values.end(), [](std::uint8_t value) { return value >= 128; });
    return static_cast<float>(marked) / static_cast<float>(values.size());
}

std::vector<std::uint8_t> stencil_values(std::span<const std::uint8_t> depth_stencil_texels) {
    std::vector<std::uint8_t> values;
    values.reserve(depth_stencil_texels.size() / 8);
    for (std::size_t index = 4; index < depth_stencil_texels.size(); index += 8) values.push_back(depth_stencil_texels[index]);
    return values;
}

RgbImage stencil_image(std::span<const std::uint8_t> values, std::uint32_t width, std::uint32_t height) {
    RgbImage image{width, height, {}};
    image.bytes.reserve(values.size() * 3);
    for (std::uint8_t value : values)
        image.bytes.insert(image.bytes.end(), {static_cast<std::uint8_t>(value * 67), static_cast<std::uint8_t>(value * 131),
            static_cast<std::uint8_t>(value * 197)});
    return image;
}

std::string stencil_histogram(std::span<const std::uint8_t> values) {
    std::array<std::size_t, 256> counts{};
    for (std::uint8_t value : values) ++counts[value];
    std::string text;
    for (int value = 0; value < 256; ++value) {
        if (counts[value] == 0) continue;
        char entry[48];
        std::snprintf(entry, sizeof(entry), "%s%d:%.3f", text.empty() ? "" : " ", value,
            static_cast<double>(counts[value]) / static_cast<double>(values.size()));
        text += entry;
    }
    return text;
}

std::string float_lines(std::span<const float> values, std::size_t per_line) {
    std::string text;
    for (std::size_t index = 0; index < values.size(); ++index) {
        char entry[32];
        std::snprintf(entry, sizeof(entry), "%.9g", static_cast<double>(values[index]));
        text += entry;
        bool last = index + 1 == values.size() || (index + 1) % per_line == 0;
        text += last ? "\n" : " ";
    }
    return text;
}

float mean_luma(const RgbImage& image) {
    std::size_t count = image.bytes.size() / 3;
    if (count == 0) return 0.0f;
    double total = 0.0;
    for (std::size_t index = 0; index < count; ++index)
        total += 0.2126 * image.bytes[index * 3] + 0.7152 * image.bytes[index * 3 + 1] + 0.0722 * image.bytes[index * 3 + 2];
    return static_cast<float>(total / static_cast<double>(count));
}
}
