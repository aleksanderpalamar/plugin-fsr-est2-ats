#include "upscale/frame_capture.hpp"
#include "common/logger.hpp"
#include "upscale/bmp_writer.hpp"

#include <wrl/client.h>

#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace neuralfx::upscale {
namespace {
using Microsoft::WRL::ComPtr;

struct Readback {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> bytes;
};

std::optional<Readback> read_texture(ID3D11DeviceContext* context, ID3D11Texture2D* texture) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    description.BindFlags = 0;
    description.MiscFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&description, nullptr, &staging))) return std::nullopt;
    context->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    Readback result{description.Width, description.Height, {}};
    std::size_t row = static_cast<std::size_t>(description.Width) * 4;
    result.bytes.resize(row * description.Height);
    for (UINT y = 0; y < description.Height; ++y)
        std::memcpy(result.bytes.data() + y * row, static_cast<const std::uint8_t*>(mapped.pData) + y * mapped.RowPitch, row);
    context->Unmap(staging.Get(), 0);
    return result;
}

template <typename Element>
std::vector<Element> as_elements(const Readback& readback) {
    std::vector<Element> elements(readback.bytes.size() / sizeof(Element));
    std::memcpy(elements.data(), readback.bytes.data(), elements.size() * sizeof(Element));
    return elements;
}

std::string describe(const FrameContract& contract, float color_luma, float output_luma, const MotionStats& motion) {
    return " render=" + std::to_string(contract.render.width) + "x" + std::to_string(contract.render.height) +
        " output=" + std::to_string(contract.output.width) + "x" + std::to_string(contract.output.height) +
        " jitter=" + std::to_string(contract.jitter.x) + "," + std::to_string(contract.jitter.y) +
        " reset=" + std::to_string(contract.reset ? 1 : 0) + " luma_in=" + std::to_string(color_luma) +
        " luma_out=" + std::to_string(output_luma) + " motion_mean_px=" + std::to_string(motion.mean_pixels) +
        " motion_max_px=" + std::to_string(motion.max_pixels) + " motion_over_8px=" + std::to_string(motion.fraction_over_eight);
}
}

bool FrameCapture::save_all(ID3D11DeviceContext* context, const FrameResources& resources, const FrameContract& contract) {
    auto color = read_texture(context, resources.color);
    auto depth = read_texture(context, resources.depth);
    auto motion = read_texture(context, resources.motion);
    auto output = read_texture(context, resources.output);
    if (!color || !depth || !motion || !output) return false;
    std::filesystem::create_directories(directory_);
    auto stem = "fsr-" + std::to_string(++sequence_);
    auto color_image = rgb_from_rgba8(color->bytes, color->width, color->height);
    auto output_image = rgb_from_rgba8(output->bytes, output->width, output->height);
    auto halves = as_elements<std::uint16_t>(*motion);
    bool saved = write_bmp(directory_ / (stem + "-color.bmp"), color_image) &&
        write_bmp(directory_ / (stem + "-output.bmp"), output_image) &&
        write_bmp(directory_ / (stem + "-depth.bmp"),
            depth_image(as_elements<float>(*depth), depth->width, depth->height, contract.camera)) &&
        write_bmp(directory_ / (stem + "-motion.bmp"), motion_image(halves, motion->width, motion->height));
    if (saved) log("Upscale capture " + stem + describe(contract, mean_luma(color_image), mean_luma(output_image), motion_stats(halves)));
    return saved;
}

void FrameCapture::write(ID3D11DeviceContext* context, const FrameResources& resources, const FrameContract& contract) noexcept {
    pending_ = false;
    try {
        if (!save_all(context, resources, contract)) log("Upscale capture failed");
    } catch (...) {
        log("Upscale capture failed");
    }
}
}
