#include "upscale/capture_images.hpp"

#include <array>
#include <cmath>
#include <iostream>

using namespace neuralfx::upscale;

namespace {
constexpr CameraParameters camera{0.2f, 1600.0f, 0.85f};

int half_case() {
    if (half_to_float(0x3C00) != 1.0f || half_to_float(0xC000) != -2.0f || half_to_float(0x0000) != 0.0f) return 10;
    return std::fabs(half_to_float(0x3555) - 0.33325f) < 1e-3f ? 0 : 11;
}

int rgba_case() {
    const std::array<std::uint8_t, 8> texels{10, 20, 30, 255, 40, 50, 60, 255};
    auto image = rgb_from_rgba8(texels, 2, 1);
    if (image.width != 2 || image.height != 1 || image.bytes.size() != 6) return 20;
    const std::array<std::uint8_t, 6> wanted{10, 20, 30, 40, 50, 60};
    return std::equal(wanted.begin(), wanted.end(), image.bytes.begin()) ? 0 : 21;
}

int depth_case() {
    const std::array<float, 3> depth{1.0f, 0.0f, 0.0125f};
    auto image = depth_image(depth, 3, 1, camera);
    if (image.bytes.size() != 9) return 30;
    if (image.bytes[0] != 255 || image.bytes[3] != 0) return 31;
    bool grey = image.bytes[6] == image.bytes[7] && image.bytes[7] == image.bytes[8];
    return grey && image.bytes[6] > 0 && image.bytes[6] < 255 ? 0 : 32;
}

int motion_case() {
    const std::array<std::uint16_t, 4> still{0, 0, 0x4800, 0xC800};
    auto image = motion_image(still, 2, 1);
    if (image.bytes.size() != 6) return 40;
    if (image.bytes[0] != 128 || image.bytes[1] != 128 || image.bytes[2] != 128) return 41;
    if (image.bytes[3] != 255 || image.bytes[4] != 0) return 42;
    auto stats = motion_stats(still);
    if (std::fabs(stats.max_pixels - 11.3137f) > 1e-3f || std::fabs(stats.mean_pixels - 5.65685f) > 1e-3f) return 43;
    return std::fabs(stats.fraction_over_eight - 0.5f) < 1e-6f ? 0 : 44;
}

int grey_case() {
    const std::array<std::uint8_t, 4> values{0, 64, 128, 255};
    auto image = grey_image(values, 2, 2);
    if (image.bytes.size() != 12 || image.bytes[3] != 64 || image.bytes[5] != 64 || image.bytes[11] != 255) return 60;
    return std::fabs(reactive_fraction(values) - 0.5f) < 1e-6f && reactive_fraction({}) == 0.0f ? 0 : 61;
}

int stencil_case() {
    std::array<std::uint8_t, 16> texels{};
    texels[4] = 3;
    texels[12] = 3;
    auto values = stencil_values(texels);
    if (values.size() != 2 || values[0] != 3 || values[1] != 3) return 70;
    auto image = stencil_image(values, 2, 1);
    if (image.bytes.size() != 6 || image.bytes[0] != image.bytes[3]) return 71;
    const std::array<std::uint8_t, 4> mixed{0, 0, 0, 2};
    auto text = stencil_histogram(mixed);
    return text == "0:0.750 2:0.250" ? 0 : 72;
}

int float_text_case() {
    const std::array<float, 5> values{1.0f, -0.5f, 0.0f, 2.25f, 0.125f};
    auto text = float_lines(values, 4);
    return text == "1 -0.5 0 2.25\n0.125\n" ? 0 : 80;
}

int luma_case() {
    RgbImage image{2, 1, {0, 0, 0, 255, 255, 255}};
    return std::fabs(mean_luma(image) - 127.5f) < 1e-3f ? 0 : 50;
}
}

int main() {
    for (auto test_case : {half_case, rgba_case, depth_case, motion_case, grey_case, stencil_case, float_text_case, luma_case}) {
        if (int code = test_case(); code != 0) {
            std::cerr << "capture_images case failed: " << code << '\n';
            return code;
        }
    }
    return 0;
}
