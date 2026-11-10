#include "common/logger.hpp"
#include "upscale/dx12/dx12_backend.hpp"
#include "upscale/upscale_session.hpp"

#include <d3d11_4.h>
#include <wrl/client.h>
#include <array>
#include <cstring>
#include <iostream>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace neuralfx::upscale;
namespace temporal = neuralfx::temporal;

namespace {
constexpr temporal::RenderSize render{640, 360};
constexpr temporal::RenderSize output{1280, 720};
constexpr int skipped = 77;

struct Host {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
};

std::optional<Host> create_host() {
    Host host;
    D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_11_1};
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 1, D3D11_SDK_VERSION,
            &host.device, nullptr, &host.context))) return std::nullopt;
    return host;
}

ComPtr<ID3D11Texture2D> texture(ID3D11Device* device, DXGI_FORMAT format, temporal::RenderSize size, UINT bind,
    const void* data, UINT pitch) {
    D3D11_TEXTURE2D_DESC description{size.width, size.height, 1, 1, format, {1, 0}, D3D11_USAGE_DEFAULT, bind, 0, 0};
    D3D11_SUBRESOURCE_DATA initial{data, pitch, 0};
    ComPtr<ID3D11Texture2D> result;
    device->CreateTexture2D(&description, data ? &initial : nullptr, &result);
    return result;
}

struct Inputs {
    ComPtr<ID3D11Texture2D> color, depth, motion, output;
    FrameResources resources() const { return {color.Get(), depth.Get(), motion.Get(), output.Get()}; }
};

std::optional<Inputs> make_inputs(ID3D11Device* device) {
    std::vector<unsigned char> color(render.width * render.height * 4);
    for (size_t index = 0; index < color.size(); ++index) color[index] = static_cast<unsigned char>(index * 5 + 40);
    std::vector<float> depth(render.width * render.height, 0.5f);
    std::vector<unsigned short> motion(render.width * render.height * 2, 0);
    Inputs inputs;
    inputs.color = texture(device, dx12_color_format, render, D3D11_BIND_SHADER_RESOURCE, color.data(), render.width * 4);
    inputs.depth = texture(device, dx12_depth_format, render, D3D11_BIND_SHADER_RESOURCE, depth.data(), render.width * 4);
    inputs.motion = texture(device, dx12_motion_format, render, D3D11_BIND_SHADER_RESOURCE, motion.data(), render.width * 4);
    inputs.output = texture(device, dx12_output_format, output, D3D11_BIND_SHADER_RESOURCE, nullptr, 0);
    if (!inputs.color || !inputs.depth || !inputs.motion || !inputs.output) return std::nullopt;
    return inputs;
}

FrameContract frame(int index) {
    FrameContract contract;
    contract.render = render;
    contract.output = output;
    contract.jitter = {0.25f - 0.1f * static_cast<float>(index % 3), -0.2f};
    contract.frame_time_ms = 16.6f;
    contract.reset = index == 0;
    return contract;
}

size_t nonzero_bytes(Host& host, ID3D11Texture2D* source) {
    D3D11_TEXTURE2D_DESC description{};
    source->GetDesc(&description);
    description.BindFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(host.device->CreateTexture2D(&description, nullptr, &staging))) return 0;
    host.context->CopyResource(staging.Get(), source);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(host.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return 0;
    size_t count = 0;
    for (UINT row = 0; row < description.Height; ++row) {
        const auto* bytes = static_cast<const unsigned char*>(mapped.pData) + static_cast<size_t>(row) * mapped.RowPitch;
        for (UINT column = 0; column < description.Width * 4; ++column) count += bytes[column] != 0;
    }
    host.context->Unmap(staging.Get(), 0);
    return count;
}

OpenParameters parameters() {
    return {render, output, true, true};
}

int strict_never_downgrades(Host& host) {
    Dx12Backend backend(host.device.Get(), host.context.Get(), neuralfx::module_directory());
    UpscaleSession session(backend, UpscaleMode::Strict);
    auto status = session.open(parameters());
    const auto& report = session.report();
    if (report.state == SessionState::Active) return report.active_version == "4.1.2" ? 0 : 20;
    return report.active_version.empty() && status != UpscaleStatus::Ready ? 0 : 21;
}

int automatic_runs(Host& host, Inputs& inputs) {
    Dx12Backend backend(host.device.Get(), host.context.Get(), neuralfx::module_directory());
    UpscaleSession session(backend, UpscaleMode::Auto);
    if (session.open(parameters()) != UpscaleStatus::Ready) return 30;
    const auto& version = session.report().active_version;
    neuralfx::log("DX12 backend active provider: " + version + " reason=" + reason_name(session.report().reason));
    if (version != "4.1.2" && version != "3.1.6") return 31;
    for (int index = 0; index < 4; ++index)
        if (session.process(frame(index), inputs.resources()) != UpscaleStatus::Ready) return 32;
    if (nonzero_bytes(host, inputs.output.Get()) == 0) return 33;
    auto wrong = frame(5);
    wrong.render = {320, 180};
    if (session.process(wrong, inputs.resources()) != UpscaleStatus::InvalidInput) return 34;
    session.close();
    return session.report().state == SessionState::Inactive ? 0 : 35;
}
}

int main() {
    auto host = create_host();
    if (!host) {
        std::cerr << "D3D11 feature level 11.1 unavailable\n";
        return skipped;
    }
    Dx12Backend probe(host->device.Get(), host->context.Get(), neuralfx::module_directory());
    if (probe.providers().empty()) {
        std::cerr << "No FSR provider reachable through D3D12 in this environment\n";
        return skipped;
    }
    auto inputs = make_inputs(host->device.Get());
    if (!inputs) return 1;
    if (int code = strict_never_downgrades(*host)) return code;
    return automatic_runs(*host, *inputs);
}
