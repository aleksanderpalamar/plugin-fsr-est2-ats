#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace neuralfx {
enum class ScreenshotPixelFormat { Rgba8, Bgra8 };

struct RgbImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

std::optional<std::uint32_t> screenshot_rgb_size(std::uint32_t width,
    std::uint32_t height) noexcept;
std::optional<RgbImage> screenshot_rgb(std::span<const std::uint8_t> bytes,
    std::uint32_t width, std::uint32_t height, std::size_t pitch,
    ScreenshotPixelFormat format);
}
