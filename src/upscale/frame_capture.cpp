#include "upscale/frame_capture.hpp"
#include "common/logger.hpp"
#include "upscale/bmp_writer.hpp"
#include "upscale/texture_readback.hpp"

#include <cstring>
#include <fstream>
#include <string>

namespace neuralfx::upscale {
namespace {
template <typename Element>
std::vector<Element> as_elements(const Readback& readback) {
    std::vector<Element> elements(readback.bytes.size() / sizeof(Element));
    std::memcpy(elements.data(), readback.bytes.data(), elements.size() * sizeof(Element));
    return elements;
}

void write_raw(const std::filesystem::path& path, const void* data, std::size_t size) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
}

std::string describe(const FrameContract& contract, float color_luma, float output_luma, const MotionStats& motion,
    float reactive) {
    return " render=" + std::to_string(contract.render.width) + "x" + std::to_string(contract.render.height) +
        " output=" + std::to_string(contract.output.width) + "x" + std::to_string(contract.output.height) +
        " jitter=" + std::to_string(contract.jitter.x) + "," + std::to_string(contract.jitter.y) +
        " reset=" + std::to_string(contract.reset ? 1 : 0) + " luma_in=" + std::to_string(color_luma) +
        " luma_out=" + std::to_string(output_luma) + " motion_mean_px=" + std::to_string(motion.mean_pixels) +
        " motion_max_px=" + std::to_string(motion.max_pixels) + " motion_over_8px=" + std::to_string(motion.fraction_over_eight) +
        " reactive_over_half=" + std::to_string(reactive);
}
}

void FrameCapture::request() noexcept {
    if (remaining_ > 0) return;
    burst_ = burst_ % capture_burst_names + 1;
    remaining_ = capture_burst_frames;
    index_ = 0;
}

void FrameCapture::save_extras(ID3D11DeviceContext* context, const CaptureSources& sources, const std::string& stem) {
    if (sources.resolve)
        if (auto bytes = read_buffer(context, sources.resolve)) {
            std::vector<float> floats(bytes->size() / sizeof(float));
            std::memcpy(floats.data(), bytes->data(), floats.size() * sizeof(float));
            auto text = float_lines(floats, 4);
            write_raw(directory_ / (stem + "-resolve.txt"), text.data(), text.size());
        }
    if (!sources.scene_depth) return;
    auto texels = read_texture(context, sources.scene_depth);
    if (!texels) return;
    auto values = stencil_values(texels->bytes);
    write_bmp(directory_ / (stem + "-stencil.bmp"), stencil_image(values, texels->width, texels->height));
    log("Upscale capture " + stem + " stencil " + stencil_histogram(values));
}

bool FrameCapture::save_frame(ID3D11DeviceContext* context, const CaptureSources& sources, const std::string& stem, bool first) {
    const auto& resources = sources.resources;
    auto color = read_texture(context, resources.color);
    auto depth = read_texture(context, resources.depth);
    auto motion = read_texture(context, resources.motion);
    auto output = read_texture(context, resources.output);
    auto reactive = resources.reactive ? read_texture(context, resources.reactive) : std::nullopt;
    if (!color || !depth || !motion || !output || (resources.reactive && !reactive)) return false;
    auto halves = as_elements<std::uint16_t>(*motion);
    auto color_image = rgb_from_rgba8(color->bytes, color->width, color->height);
    auto output_image = rgb_from_rgba8(output->bytes, output->width, output->height);
    write_raw(directory_ / (stem + "-depth.f32"), depth->bytes.data(), depth->bytes.size());
    write_raw(directory_ / (stem + "-motion.f16"), motion->bytes.data(), motion->bytes.size());
    bool saved = write_bmp(directory_ / (stem + "-color.bmp"), color_image) &&
        write_bmp(directory_ / (stem + "-output.bmp"), output_image);
    if (first) {
        saved = saved && write_bmp(directory_ / (stem + "-depth.bmp"),
            depth_image(as_elements<float>(*depth), depth->width, depth->height, sources.contract.camera)) &&
            write_bmp(directory_ / (stem + "-motion.bmp"), motion_image(halves, motion->width, motion->height));
        if (resources.previous_color)
            if (auto previous = read_texture(context, resources.previous_color))
                saved = saved && write_bmp(directory_ / (stem + "-previous.bmp"),
                    rgb_from_rgba8(previous->bytes, previous->width, previous->height));
        if (reactive)
            saved = saved && write_bmp(directory_ / (stem + "-reactive.bmp"), grey_image(reactive->bytes, reactive->width, reactive->height));
    }
    float marked = reactive ? reactive_fraction(reactive->bytes) : 0.0f;
    if (saved) log("Upscale capture " + stem + describe(sources.contract, mean_luma(color_image), mean_luma(output_image),
        motion_stats(halves), marked));
    return saved;
}

void FrameCapture::write(ID3D11DeviceContext* context, const CaptureSources& sources) noexcept {
    try {
        std::filesystem::create_directories(directory_);
        auto stem = "fsr-" + std::to_string(burst_) + "-" + std::to_string(++index_);
        bool first = index_ == 1;
        bool saved = save_frame(context, sources, stem, first);
        if (saved) save_extras(context, sources, stem);
        else log("Upscale capture failed");
        remaining_ = saved ? remaining_ - 1 : 0;
    } catch (...) {
        remaining_ = 0;
        log("Upscale capture failed");
    }
}
}
