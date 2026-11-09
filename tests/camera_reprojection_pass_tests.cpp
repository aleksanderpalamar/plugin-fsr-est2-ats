#include "common/logger.hpp"
#include "d3d11_test_device.hpp"
#include "renderer/camera_reprojection_pass.hpp"
#include "temporal/motion_reprojection.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx;
using namespace neuralfx::testing;
using namespace neuralfx::temporal;

namespace {
constexpr RenderSize scene{1440, 1080};

constexpr Matrix4 captured_view{-0.47095215f, 0.0f, -0.8821588f, -46.862404f,
    0.0f, 1.0f, 0.0f, 10.617724f,
    0.8821588f, 0.0f, -0.47095215f, -48.97045f,
    0.0f, 0.0f, 0.0f, 1.0f};

constexpr Matrix4 captured_view_projection{-0.5872703f, 0.0f, -1.1014271f, -58.526123f,
    -0.0006845583f, 2.2190397f, 0.0003654605f, 23.599154f,
    0.00011026186f, 0.0f, -5.8864753e-05f, 0.19390413f,
    -0.8821588f, 0.0f, 0.47095215f, 48.97045f};

Matrix4 moved(const Matrix4& view, float angle, float x, float z) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    Matrix4 motion{c, 0.0f, s, -x, 0.0f, 1.0f, 0.0f, 0.0f, -s, 0.0f, c, -z, 0.0f, 0.0f, 0.0f, 1.0f};
    return multiply(motion, view);
}

CameraMatrices jittered(const Matrix4& view, Jitter jitter) {
    auto base = camera_state({captured_view, captured_view_projection}, scene);
    Matrix4 projection = base->projection;
    projection[2] = -jitter.x * 2.0f / static_cast<float>(scene.width);
    projection[6] = jitter.y * 2.0f / static_cast<float>(scene.height);
    return {view, multiply(projection, view)};
}

ComPtr<ID3D11Buffer> game_camera(ID3D11Device* device, const CameraMatrices& camera) {
    std::array<float, 32> values{};
    std::copy(camera.view.begin(), camera.view.end(), values.begin());
    std::copy(camera.view_projection.begin(), camera.view_projection.end(), values.begin() + 16);
    D3D11_BUFFER_DESC description{sizeof(values), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA data{values.data(), 0, 0};
    ComPtr<ID3D11Buffer> buffer;
    device->CreateBuffer(&description, &data, &buffer);
    return buffer;
}

std::optional<GpuCameraFrame> read_frame(const TestDevice& gpu, ID3D11ShaderResourceView* view) {
    ComPtr<ID3D11Resource> resource;
    view->GetResource(&resource);
    D3D11_BUFFER_DESC description{sizeof(GpuCameraFrame), D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
    ComPtr<ID3D11Buffer> staging;
    if (FAILED(gpu.device->CreateBuffer(&description, nullptr, &staging))) return std::nullopt;
    gpu.context->CopyResource(staging.Get(), resource.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    GpuCameraFrame frame;
    std::memcpy(&frame, mapped.pData, sizeof(frame));
    gpu.context->Unmap(staging.Get(), 0);
    return frame;
}

std::optional<GpuCameraFrame> run(const TestDevice& gpu, CameraReprojectionPass& pass,
    const CameraMatrices& camera, bool history) {
    auto buffer = game_camera(gpu.device.Get(), camera);
    if (!buffer) return std::nullopt;
    auto* view = pass.run(gpu.context.Get(), buffer.Get(), scene, history);
    return view ? read_frame(gpu, view) : std::nullopt;
}

float largest_error(const MotionConstants& expected, const GpuCameraFrame& frame) {
    MotionConstants actual{frame.reprojection, {frame.jitter[0], frame.jitter[1]}, scene};
    float largest = 0.0f;
    for (float distance : {0.3f, 1.0f, 5.0f, 40.0f, 900.0f}) {
        float depth = 0.20002499f / distance;
        for (float y = 0.5f; y < 1080.0f; y += 179.0f) {
            for (float x = 0.5f; x < 1440.0f; x += 239.0f) {
                auto want = camera_motion(expected, x, y, depth);
                auto got = camera_motion(actual, x, y, depth);
                if (!want || !got) return 1e9f;
                largest = std::max({largest, std::fabs(want->x - got->x), std::fabs(want->y - got->y)});
            }
        }
    }
    return largest;
}

int expect_motion(const TestDevice& gpu, CameraReprojectionPass& pass, const CameraMatrices& previous,
    const CameraMatrices& current, float tolerance, int code) {
    if (!run(gpu, pass, previous, false)) return code;
    auto frame = run(gpu, pass, current, true);
    auto expected = motion_constants(*camera_state(previous, scene), *camera_state(current, scene), scene);
    if (!frame || !expected || frame->valid != 1) return code + 1;
    float error = largest_error(*expected, *frame);
    if (error <= tolerance) return 0;
    std::cerr << "case " << code << " largest motion error " << error << " px\n";
    return code + 2;
}

int jitter_case(const TestDevice& gpu, CameraReprojectionPass& pass) {
    auto frame = run(gpu, pass, {captured_view, captured_view_projection}, false);
    if (!frame || frame->valid != 1) return 10;
    if (std::fabs(frame->jitter[0] + 0.470703f) > 1e-3f || std::fabs(frame->jitter[1] + 0.419042f) > 1e-3f) return 11;
    auto still = camera_state({captured_view, captured_view_projection}, scene);
    float error = largest_error(*motion_constants(*still, *still, scene), *frame);
    if (error < 1e-2f) return 0;
    std::cerr << "still camera largest motion error " << error << " px\n";
    return 12;
}

int motion_cases(const TestDevice& gpu, CameraReprojectionPass& pass) {
    auto previous = jittered(captured_view, {-0.4707f, -0.4190f});
    auto turned = jittered(moved(captured_view, 0.02f, 0.3f, -1.2f), {0.2841f, 0.0111f});
    if (int code = expect_motion(gpu, pass, previous, turned, 0.01f, 20)) return code;
    Matrix4 far_view = captured_view;
    far_view[3] -= 4000.0f;
    far_view[11] += 2500.0f;
    auto far_previous = jittered(far_view, {-0.4707f, -0.4190f});
    auto far_current = jittered(moved(far_view, 0.01f, 0.5f, -0.8f), {0.0390f, 0.4413f});
    return expect_motion(gpu, pass, far_previous, far_current, 0.25f, 30);
}

int singular_case(const TestDevice& gpu, CameraReprojectionPass& pass) {
    auto frame = run(gpu, pass, {Matrix4{}, captured_view_projection}, false);
    return frame && frame->valid == 0 ? 0 : 40;
}
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) {
        std::cerr << "D3D11 device unavailable\n";
        return 1;
    }
    CameraReprojectionPass pass;
    if (FAILED(pass.initialize(gpu->device.Get(), module_directory() / L"NeuralFX" / L"shaders"))) {
        std::cerr << "Camera reprojection pass initialization failed\n";
        return 2;
    }
    if (int code = jitter_case(*gpu, pass)) return code;
    if (int code = motion_cases(*gpu, pass)) return code;
    return singular_case(*gpu, pass);
}
