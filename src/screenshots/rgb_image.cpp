#include "screenshots/rgb_image.hpp"

#include <limits>

namespace neuralfx {
std::optional<std::uint32_t> screenshot_rgb_size(std::uint32_t width,
    std::uint32_t height) noexcept {
    if (width == 0 || height == 0) return std::nullopt;
    if (width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max())
        return std::nullopt;
    auto bytes = static_cast<std::uint64_t>(width) * height * 3;
    if (bytes > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    return static_cast<std::uint32_t>(bytes);
}

std::optional<RgbImage> screenshot_rgb(std::span<const std::uint8_t> bytes,
    std::uint32_t width, std::uint32_t height, std::size_t pitch,
    ScreenshotPixelFormat format) {
    auto size = screenshot_rgb_size(width, height);
    if (!size) return std::nullopt;
    if (format != ScreenshotPixelFormat::Rgba8 && format != ScreenshotPixelFormat::Bgra8)
        return std::nullopt;
    auto row_bytes = static_cast<std::size_t>(width) * 4;
    if (pitch < row_bytes || bytes.size() < row_bytes) return std::nullopt;
    if (height - 1 > (bytes.size() - row_bytes) / pitch) return std::nullopt;
    RgbImage image{static_cast<int>(width), static_cast<int>(height),
        std::vector<std::uint8_t>(*size)};
    std::size_t red = format == ScreenshotPixelFormat::Rgba8 ? 0 : 2;
    for (std::uint32_t y = 0; y < height; ++y) {
        auto source = bytes.subspan(static_cast<std::size_t>(y) * pitch, row_bytes);
        auto output = static_cast<std::size_t>(y) * width * 3;
        for (std::uint32_t x = 0; x < width; ++x) {
            auto input = static_cast<std::size_t>(x) * 4;
            image.pixels[output++] = source[input + red];
            image.pixels[output++] = source[input + 1];
            image.pixels[output++] = source[input + 2 - red];
        }
    }
    return image;
}
}
