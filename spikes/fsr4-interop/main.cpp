#include "d3d11_source.hpp"
#include "d3d12_readback.hpp"
#include "fsr_probe.hpp"
#include "report.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
using namespace spike;

struct Format {
    const char* name;
    DXGI_FORMAT format;
    UINT bytes;
};

constexpr Format formats[]{
    {"rgba16f", DXGI_FORMAT_R16G16B16A16_FLOAT, 8},
    {"rgba8", DXGI_FORMAT_R8G8B8A8_UNORM, 4},
    {"r32f", DXGI_FORMAT_R32_FLOAT, 4},
    {"rg16f", DXGI_FORMAT_R16G16_FLOAT, 4},
};

std::vector<unsigned char> pattern(UINT size, UINT bytes) {
    std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * bytes);
    for (size_t index = 0; index < pixels.size(); ++index) pixels[index] = static_cast<unsigned char>(index * 7 + 3);
    return pixels;
}

template <typename Value>
std::vector<unsigned char> filled(UINT width, UINT height, std::initializer_list<Value> texel) {
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * texel.size() * sizeof(Value));
    for (size_t offset = 0; offset < pixels.size(); offset += texel.size() * sizeof(Value))
        std::memcpy(pixels.data() + offset, texel.begin(), texel.size() * sizeof(Value));
    return pixels;
}

bool roundtrip(D3D11Source& source, D3D12Sink& sink, const Format& format) {
    constexpr UINT size = 64;
    auto pixels = pattern(size, format.bytes);
    auto shared = create_shared(source, format.name, format.format, size, size, pixels, size * format.bytes);
    if (!shared) return false;
    auto opened = open_texture(sink, shared->handle, format.name);
    if (!opened) return false;
    auto readback = read_texture(sink, opened.Get(), D3D12_RESOURCE_STATE_COMMON, signal(source));
    bool match = readback && readback->pixels == pixels;
    std::string step = std::string("interop.roundtrip.") + format.name;
    return report(step.c_str(), match ? S_OK : E_FAIL,
        readback ? "copy_gpu_ms=" + std::to_string(readback->milliseconds) : "readback_failed");
}

ComPtr<ID3D12Resource> shared_input(D3D11Source& source, D3D12Sink& sink, const char* name, DXGI_FORMAT format,
    UINT width, UINT height, const std::vector<unsigned char>& pixels, UINT bytes) {
    auto shared = create_shared(source, name, format, width, height, pixels, width * bytes);
    return shared ? open_texture(sink, shared->handle, name) : nullptr;
}

bool fsr(D3D11Source& source, D3D12Sink& sink) {
    constexpr UINT width = 640;
    constexpr UINT height = 360;
    auto color = shared_input(source, sink, "fsr_color", DXGI_FORMAT_R16G16B16A16_FLOAT, width, height,
        filled<uint16_t>(width, height, {0x3800, 0x3400, 0x3000, 0x3C00}), 8);
    auto depth = shared_input(source, sink, "fsr_depth", DXGI_FORMAT_R32_FLOAT, width, height,
        filled<float>(width, height, {0.5f}), 4);
    auto motion = shared_input(source, sink, "fsr_motion", DXGI_FORMAT_R16G16_FLOAT, width, height,
        filled<uint16_t>(width, height, {0, 0}), 4);
    if (!color || !depth || !motion) return false;
    return probe_fsr(sink, {color.Get(), depth.Get(), motion.Get(), width, height}, signal(source));
}
}

int main() {
    if (!std::freopen("fsr4_interop_probe.log", "w", stdout)) return 4;
    auto source = create_source();
    if (!source) return 1;
    auto sink = create_sink(source->adapter, source->fence_handle);
    if (!sink) return 2;
    bool interop = true;
    for (const auto& format : formats) interop = roundtrip(*source, *sink, format) && interop;
    bool upscaler = fsr(*source, *sink);
    std::printf("summary interop=%s fsr=%s\n", interop ? "ok" : "fail", upscaler ? "ok" : "fail");
    return interop && upscaler ? 0 : 3;
}
