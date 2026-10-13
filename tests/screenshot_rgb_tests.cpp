#include "screenshots/rgb_image.hpp"

#include <array>
#include <limits>

int main() {
    using namespace neuralfx;
    std::array<std::uint8_t, 24> rgba{
        255, 0, 0, 5, 0, 255, 0, 7, 99, 99, 99, 99,
        0, 0, 255, 9, 255, 255, 255, 11, 99, 99, 99, 99};
    std::vector<std::uint8_t> expected{255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255};
    auto image = screenshot_rgb(rgba, 2, 2, 12, ScreenshotPixelFormat::Rgba8);
    if (!image || image->width != 2 || image->height != 2 || image->pixels != expected) return 1;
    std::array<std::uint8_t, 24> bgra{
        0, 0, 255, 5, 0, 255, 0, 7, 99, 99, 99, 99,
        255, 0, 0, 9, 255, 255, 255, 11, 99, 99, 99, 99};
    image = screenshot_rgb(bgra, 2, 2, 12, ScreenshotPixelFormat::Bgra8);
    if (!image || image->pixels != expected) return 2;
    if (screenshot_rgb(rgba, 0, 2, 12, ScreenshotPixelFormat::Rgba8)) return 3;
    if (screenshot_rgb(rgba, 2, 0, 12, ScreenshotPixelFormat::Rgba8)) return 4;
    if (screenshot_rgb(rgba, 2, 2, 7, ScreenshotPixelFormat::Rgba8)) return 5;
    if (screenshot_rgb(std::span(rgba).first(19), 2, 2, 12, ScreenshotPixelFormat::Rgba8)) return 6;
    if (screenshot_rgb(rgba, 2, 2, std::numeric_limits<std::size_t>::max(),
        ScreenshotPixelFormat::Rgba8)) return 7;
    if (screenshot_rgb_size(65536, 65536)) return 8;
    if (screenshot_rgb_size(std::numeric_limits<std::uint32_t>::max(), 1)) return 9;
    if (screenshot_rgb_size(0, 1)) return 10;
    if (screenshot_rgb_size(1920, 1080) != 6220800) return 11;
    if (screenshot_rgb(rgba, 2, 2, 12, static_cast<ScreenshotPixelFormat>(99))) return 12;
    return 0;
}
