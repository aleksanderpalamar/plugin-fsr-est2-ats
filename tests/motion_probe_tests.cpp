#include "common/logger.hpp"
#include "diagnostics/motion_probe.hpp"
#include "dxgi/context_hook.hpp"
#include "scene_frame_fixture.hpp"
#include "temporal/camera_state.hpp"
#include "temporal/jitter_sequence.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>

using namespace scene_test;
namespace diagnostics = neuralfx::diagnostics;
namespace temporal = neuralfx::temporal;

namespace {
constexpr CameraValues captured_camera{-0.47095215f, 0.0f, -0.8821588f, -46.862404f,
    0.0f, 1.0f, 0.0f, 10.617724f,
    0.8821588f, 0.0f, -0.47095215f, -48.97045f,
    0.0f, 0.0f, 0.0f, 1.0f,
    -0.5872703f, 0.0f, -1.1014271f, -58.526123f,
    -0.0006845583f, 2.2190397f, 0.0003654605f, 23.599154f,
    0.00011026186f, 0.0f, -5.8864753e-05f, 0.19390413f,
    -0.8821588f, 0.0f, 0.47095215f, 48.97045f};

std::filesystem::path log_file() {
    return neuralfx::module_directory() / L"NeuralFX" / L"logs" / L"neuralfx.log";
}

std::uintmax_t log_size() {
    std::error_code error;
    auto size = std::filesystem::file_size(log_file(), error);
    return error ? 0 : size;
}

std::string log_since(std::uintmax_t offset) {
    std::ifstream file(log_file(), std::ios::binary);
    file.seekg(static_cast<std::streamoff>(offset));
    std::stringstream content;
    content << file.rdbuf();
    return content.str();
}

CameraValues camera_for(std::uint32_t phase, float shift) {
    temporal::Matrix4 view;
    temporal::Matrix4 view_projection;
    std::copy(captured_camera.begin(), captured_camera.begin() + 16, view.begin());
    std::copy(captured_camera.begin() + 16, captured_camera.end(), view_projection.begin());
    temporal::RenderSize size{render_width, render_height};
    auto projection = temporal::camera_state({view, view_projection}, size)->projection;
    view[3] += shift;
    auto jitter = temporal::sequence_jitter(phase);
    projection[2] = -jitter.x * 2.0f / static_cast<float>(render_width);
    projection[6] = jitter.y * 2.0f / static_cast<float>(render_height);
    auto jittered = temporal::multiply(projection, view);
    CameraValues values{};
    std::copy(view.begin(), view.end(), values.begin());
    std::copy(jittered.begin(), jittered.end(), values.begin() + 16);
    return values;
}

void upload_camera(Device& gpu, const Targets& targets, std::uint32_t phase, float shift) {
    auto values = camera_for(phase, shift);
    D3D11_BOX box{camera_offset * 16, 0, 0, camera_offset * 16 + static_cast<UINT>(sizeof(values)), 1, 1};
    gpu.context->UpdateSubresource(targets.camera.Get(), 0, &box, values.data(), 0, 0);
}

void request_toggle() {
    std::ofstream(neuralfx::module_directory() / L"NeuralFX" / L"motion-debug").put('\n');
}

void present(Device& gpu, const Targets& targets, ID3D11RenderTargetView* output, std::uint32_t phase,
    float shift) {
    const float black[4]{0.0f, 0.0f, 0.0f, 1.0f};
    upload_camera(gpu, targets, phase, shift);
    gpu.context->ClearRenderTargetView(output, black);
    gpu.context->ClearDepthStencilView(targets.depth_view.Get(), D3D11_CLEAR_DEPTH, 0.5f, 0);
    game_frame(gpu, targets, true);
    diagnostics::present_motion_probe(gpu.swapchain.Get());
    diagnostics::motion_probe_frame_end(gpu.swapchain.Get());
    gpu.context->Flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
}

std::optional<std::array<std::uint8_t, 4>> pixel(Device& gpu, ID3D11Texture2D* texture, UINT x, UINT y) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    description.BindFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(gpu.device->CreateTexture2D(&description, nullptr, &staging))) return std::nullopt;
    gpu.context->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    std::array<std::uint8_t, 4> value{};
    std::memcpy(value.data(), static_cast<const std::uint8_t*>(mapped.pData) + y * mapped.RowPitch + x * 4, 4);
    gpu.context->Unmap(staging.Get(), 0);
    return value;
}

int check(Device& gpu, ID3D11Texture2D* backbuffer) {
    auto inside = pixel(gpu, backbuffer, 4, 4);
    auto outside = pixel(gpu, backbuffer, 24, 24);
    if (!inside || !outside) return 20;
    for (int channel = 0; channel < 3; ++channel) {
        if (std::abs((*inside)[channel] - 128) > 2) return 21;
        if ((*outside)[channel] != 0) return 22;
    }
    return 0;
}
}

int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW, 0, 0, 32, 32,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    Device gpu;
    Pipeline pipeline;
    Targets targets;
    if (!window || FAILED(create_device(window, gpu))) return 1;
    if (!create_pipeline(gpu.device.Get(), pipeline) || !create_targets(gpu.device.Get(), captured_camera, targets))
        return 2;
    bind_pipeline(gpu.context.Get(), pipeline);
    neuralfx::hook_context(gpu.swapchain.Get());
    ComPtr<ID3D11Texture2D> backbuffer;
    ComPtr<ID3D11RenderTargetView> output;
    gpu.swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    if (FAILED(gpu.device->CreateRenderTargetView(backbuffer.Get(), nullptr, &output))) return 3;
    auto offset = log_size();
    request_toggle();
    diagnostics::motion_probe_frame_end(gpu.swapchain.Get());
    for (std::uint32_t phase = 3; phase < 3 + 16; ++phase)
        present(gpu, targets, output.Get(), phase, phase < 11 ? 0.0f : 10.0f);
    int result = check(gpu, backbuffer.Get());
    auto log = log_since(offset);
    if (result == 0 && log.find("status=ready") == std::string::npos) result = 30;
    if (result == 0 && log.find("sync=confirmed@") == std::string::npos) result = 31;
    if (result == 0 && log.find("wait_us") != std::string::npos) result = 32;
    if (result == 0 && log.find("status=camera-cut") == std::string::npos) result = 33;
    if (result == 0 && log.find("Motion camera jump") == std::string::npos) result = 34;
    if (result == 0 && log.find("Motion camera rejected") != std::string::npos) result = 35;
    DestroyWindow(window);
    return result;
}
