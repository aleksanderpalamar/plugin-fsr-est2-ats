#include "common/logger.hpp"
#include "d3d11_test_device.hpp"
#include "renderer/camera_motion_pass.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx;
using namespace neuralfx::testing;
using namespace neuralfx::temporal;

namespace {
constexpr UINT width = 64;
constexpr UINT height = 48;
constexpr RenderSize size{width, height};

constexpr Matrix4 projection{1.2482098f, 0.0f, 0.0f, 0.0f,
    0.0f, 2.2190397f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.00012499095f, 0.20002499f,
    0.0f, 0.0f, -1.0f, 0.0f};

Matrix4 translation(float x, float y, float z) {
    return {1.0f, 0.0f, 0.0f, -x, 0.0f, 1.0f, 0.0f, -y, 0.0f, 0.0f, 1.0f, -z, 0.0f, 0.0f, 0.0f, 1.0f};
}

CameraState camera(const Matrix4& view, Jitter jitter) {
    return {jitter, projection, multiply(projection, view)};
}

std::vector<float> scene_depth() {
    std::vector<float> depth(static_cast<size_t>(width) * height);
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            float distance = 4.0f + static_cast<float>(x + 2 * y) * 0.75f;
            depth[static_cast<size_t>(y) * width + x] = -projection[10] + projection[11] / distance;
        }
    }
    return depth;
}

int compare(const TestDevice& gpu, CameraMotionPass& pass, const MotionConstants& constants, int code) {
    auto depth = scene_depth();
    auto input = create_input(gpu.device.Get(), width, height, DXGI_FORMAT_R32_FLOAT, depth.data(),
        width * sizeof(float));
    auto target = create_float_target(gpu.device.Get(), width, height);
    if (!input || !target) return code;
    pass.render(gpu.context.Get(), {input.Get(), target->view.Get()}, constants);
    auto pixels = read_pixels(gpu, target->texture.Get(), width, height);
    if (!pixels) return code + 1;
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            size_t index = static_cast<size_t>(y) * width + x;
            auto expected = camera_motion(constants, x + 0.5f, y + 0.5f, depth[index]);
            PixelMotion wanted = expected.value_or(PixelMotion{});
            const auto& actual = (*pixels)[index];
            if (std::fabs(actual[0] - wanted.x) > 1e-2f || std::fabs(actual[1] - wanted.y) > 1e-2f) {
                std::cerr << "pixel " << x << ',' << y << ": " << actual[0] << ',' << actual[1]
                          << " expected " << wanted.x << ',' << wanted.y << '\n';
                return code + 2;
            }
        }
    }
    return 0;
}

int run_cases(const TestDevice& gpu, CameraMotionPass& pass) {
    auto still = motion_constants(camera(translation(0.0f, 0.0f, 0.0f), {0.3f, -0.2f}),
        camera(translation(0.0f, 0.0f, 0.0f), {-0.47f, -0.42f}), size);
    auto moving = motion_constants(camera(translation(0.0f, 0.0f, 0.0f), {0.28f, 0.01f}),
        camera(translation(0.5f, 0.2f, -1.5f), {-0.47f, -0.42f}), size);
    if (!still || !moving) return 10;
    if (int code = compare(gpu, pass, *still, 20)) return code;
    return compare(gpu, pass, *moving, 30);
}
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) {
        std::cerr << "D3D11 device unavailable\n";
        return 1;
    }
    CameraMotionPass pass;
    if (FAILED(pass.initialize(gpu->device.Get(), module_directory() / L"NeuralFX" / L"shaders"))) {
        std::cerr << "Camera motion pass initialization failed\n";
        return 2;
    }
    return run_cases(*gpu, pass);
}
