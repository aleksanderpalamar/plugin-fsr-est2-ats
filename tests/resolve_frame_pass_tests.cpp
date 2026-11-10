#include "common/logger.hpp"
#include "d3d11_test_device.hpp"
#include "renderer/resolve_frame_pass.hpp"
#include "temporal/motion_reprojection.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx;
using namespace neuralfx::testing;
using namespace neuralfx::temporal;

namespace {
constexpr RenderSize scene{1440, 1080};
using Values = std::array<float, 64>;

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

Matrix4 raster_projection() {
    Matrix4 projection = camera_state({captured_view, captured_view_projection}, scene)->projection;
    projection[2] = 0.0f;
    projection[6] = 0.0f;
    return projection;
}

Matrix4 resolve_projection() {
    Matrix4 projection = raster_projection();
    projection[10] = -(1.0f + projection[10]);
    projection[11] = -projection[11];
    return projection;
}

CameraMatrices camera_at(const Matrix4& view) {
    return {view, multiply(raster_projection(), view)};
}

void put_rows(Values& values, const Matrix4& matrix, std::size_t first_register) {
    std::copy(matrix.begin(), matrix.end(), values.begin() + first_register * 4);
}

Values game_resolve(const Matrix4& previous_view, const Matrix4& current_view, Jitter jitter) {
    Matrix4 relative = multiply(previous_view, *inverse(current_view));
    Values values{};
    put_rows(values, multiply(resolve_projection(), relative), 0);
    put_rows(values, relative, 4);
    values[32] = jitter.x / static_cast<float>(scene.width);
    values[33] = jitter.y / static_cast<float>(scene.height);
    values[34] = 0.1f;
    values[35] = 0.1f;
    values[44] = static_cast<float>(scene.width);
    values[45] = static_cast<float>(scene.height);
    values[46] = 1.0f;
    values[47] = 1.0f;
    return values;
}

ComPtr<ID3D11Buffer> upload(ID3D11Device* device, const Values& values) {
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

std::optional<GpuCameraFrame> run(const TestDevice& gpu, ResolveFramePass& pass, const Values& values,
    RenderSize size = scene) {
    auto buffer = upload(gpu.device.Get(), values);
    if (!buffer) return std::nullopt;
    auto* view = pass.run(gpu.context.Get(), buffer.Get(), size);
    return view ? read_frame(gpu, view) : std::nullopt;
}

float largest_error(const MotionConstants& expected, const GpuCameraFrame& frame) {
    MotionConstants actual{frame.reprojection, {frame.jitter[0], frame.jitter[1]}, scene};
    Matrix4 projection = raster_projection();
    float largest = 0.0f;
    for (float distance : {0.3f, 1.0f, 5.0f, 40.0f, 900.0f}) {
        float depth = projection[11] / distance - projection[10];
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

int expect_motion(const TestDevice& gpu, ResolveFramePass& pass, const Matrix4& previous, const Matrix4& current,
    Jitter jitter, float tolerance, int code) {
    auto frame = run(gpu, pass, game_resolve(previous, current, jitter));
    auto expected = motion_constants(*camera_state(camera_at(previous), scene), *camera_state(camera_at(current), scene),
        scene);
    if (!frame || !expected || frame->valid != 1) return code + 1;
    expected->jitter = jitter;
    float error = largest_error(*expected, *frame);
    if (error <= tolerance) return 0;
    std::cerr << "case " << code << " largest motion error " << error << " px\n";
    return code + 2;
}

int jitter_case(const TestDevice& gpu, ResolveFramePass& pass) {
    auto frame = run(gpu, pass, game_resolve(captured_view, captured_view, {-0.4707f, -0.4190f}));
    if (!frame || frame->valid != 1) return 10;
    if (std::fabs(frame->jitter[0] + 0.4707f) > 1e-4f || std::fabs(frame->jitter[1] + 0.4190f) > 1e-4f) return 11;
    return expect_motion(gpu, pass, captured_view, captured_view, {-0.4707f, -0.4190f}, 0.05f, 12);
}

int motion_cases(const TestDevice& gpu, ResolveFramePass& pass) {
    auto turned = moved(captured_view, 0.02f, 0.3f, -1.2f);
    if (int code = expect_motion(gpu, pass, captured_view, turned, {0.2841f, 0.0111f}, 0.05f, 20)) return code;
    auto driving = moved(captured_view, 0.0f, 0.0f, -0.3f);
    if (int code = expect_motion(gpu, pass, captured_view, driving, {0.0390f, 0.4413f}, 0.05f, 30)) return code;
    auto steering = moved(captured_view, 0.01f, 0.0f, -0.3f);
    return expect_motion(gpu, pass, captured_view, steering, {-0.2451f, 0.4301f}, 0.05f, 40);
}

int invalid_case(const TestDevice& gpu, ResolveFramePass& pass, const Values& values, int code) {
    auto frame = run(gpu, pass, values);
    return frame && frame->valid == 0 ? 0 : code;
}

int validity_cases(const TestDevice& gpu, ResolveFramePass& pass) {
    if (int code = invalid_case(gpu, pass, game_resolve(captured_view, moved(captured_view, 0.0f, 0.0f, -5.0f), {}), 50))
        return code;
    if (int code = invalid_case(gpu, pass, game_resolve(captured_view, moved(captured_view, 0.6f, 0.0f, 0.0f), {}), 51))
        return code;
    Values broken = game_resolve(captured_view, captured_view, {});
    broken[33] = std::nanf("");
    if (int code = invalid_case(gpu, pass, broken, 52)) return code;
    Values skewed = game_resolve(captured_view, captured_view, {});
    skewed[31] = 0.5f;
    skewed[27] = 0.5f;
    if (int code = invalid_case(gpu, pass, skewed, 53)) return code;
    auto frame = run(gpu, pass, game_resolve(captured_view, captured_view, {}), {1920, 1080});
    return frame && frame->valid == 0 ? 0 : 54;
}
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) {
        std::cerr << "D3D11 device unavailable\n";
        return 1;
    }
    ResolveFramePass pass;
    if (FAILED(pass.initialize(gpu->device.Get(), module_directory() / L"NeuralFX" / L"shaders"))) {
        std::cerr << "Resolve frame pass initialization failed\n";
        return 2;
    }
    if (int code = jitter_case(*gpu, pass)) return code;
    if (int code = motion_cases(*gpu, pass)) return code;
    return validity_cases(*gpu, pass);
}
