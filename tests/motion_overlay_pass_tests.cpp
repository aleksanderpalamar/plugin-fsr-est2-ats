#include "common/logger.hpp"
#include "d3d11_test_device.hpp"
#include "renderer/motion_overlay_pass.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx;
using namespace neuralfx::testing;

namespace {
constexpr UINT motion_width = 8;
constexpr UINT motion_height = 6;
constexpr UINT overlay_width = 16;
constexpr UINT overlay_height = 12;

std::vector<std::array<float, 2>> motion_field() {
    std::vector<std::array<float, 2>> field(motion_width * motion_height);
    for (UINT y = 0; y < motion_height; ++y)
        for (UINT x = motion_width / 2; x < motion_width; ++x) field[y * motion_width + x] = {4.0f, -2.0f};
    return field;
}

bool close_to(float actual, float expected) {
    return std::fabs(actual - expected) < 2e-3f;
}

int check(const std::vector<Pixel>& pixels) {
    for (UINT y = 0; y < overlay_height; ++y) {
        for (UINT x = 0; x < overlay_width; ++x) {
            const auto& pixel = pixels[y * overlay_width + x];
            bool moving = x >= overlay_width / 2;
            float red = moving ? 0.7f : 0.5f;
            float green = moving ? 0.4f : 0.5f;
            if (!close_to(pixel[0], red) || !close_to(pixel[1], green) || !close_to(pixel[2], 0.5f)) {
                std::cerr << "pixel " << x << ',' << y << ": " << pixel[0] << ',' << pixel[1] << ',' << pixel[2] << '\n';
                return 10;
            }
        }
    }
    return 0;
}
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) return 1;
    MotionOverlayPass pass;
    if (FAILED(pass.initialize(gpu->device.Get(), module_directory() / L"NeuralFX" / L"shaders"))) {
        std::cerr << "Motion overlay initialization failed\n";
        return 2;
    }
    auto field = motion_field();
    auto motion = create_input(gpu->device.Get(), motion_width, motion_height, DXGI_FORMAT_R32G32_FLOAT,
        field.data(), motion_width * sizeof(field[0]));
    auto target = create_float_target(gpu->device.Get(), overlay_width, overlay_height);
    if (!motion || !target) return 3;
    pass.render(gpu->context.Get(), {motion.Get(), target->view.Get(), overlay_width, overlay_height}, 0.05f);
    auto pixels = read_pixels(*gpu, target->texture.Get(), overlay_width, overlay_height);
    return pixels ? check(*pixels) : 4;
}
