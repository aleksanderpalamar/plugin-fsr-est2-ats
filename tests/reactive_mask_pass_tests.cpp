#include "common/logger.hpp"
#include "d3d11_test_device.hpp"
#include "renderer/reactive_mask_pass.hpp"

#include <cstdint>
#include <iostream>

using namespace neuralfx;
using namespace neuralfx::testing;

namespace {
constexpr UINT size = 16;
constexpr std::uint8_t dark = 13;
constexpr std::uint8_t left_grey = 77;
constexpr std::uint8_t right_grey = 179;
constexpr UINT edge = 8;

using Image = std::vector<std::uint8_t>;

Image scene(UINT edge_at) {
    Image texels(size * size * 4, 255);
    for (UINT y = 0; y < size; ++y)
        for (UINT x = 0; x < size; ++x)
            for (UINT channel = 0; channel < 3; ++channel)
                texels[(y * size + x) * 4 + channel] = x < edge_at ? left_grey : right_grey;
    return texels;
}

void paint_block(Image& texels, UINT x0, UINT y0, UINT extent) {
    for (UINT y = y0; y < y0 + extent; ++y)
        for (UINT x = x0; x < x0 + extent; ++x)
            for (UINT channel = 0; channel < 3; ++channel) texels[(y * size + x) * 4 + channel] = dark;
}

ComPtr<ID3D11ShaderResourceView> color_view(ID3D11Device* device, const Image& texels) {
    return create_input(device, size, size, DXGI_FORMAT_R8G8B8A8_UNORM, texels.data(), size * 4);
}

ComPtr<ID3D11ShaderResourceView> motion_view(ID3D11Device* device, float x, float y) {
    std::vector<float> vectors(size * size * 2);
    for (UINT index = 0; index < size * size; ++index) {
        vectors[index * 2] = x;
        vectors[index * 2 + 1] = y;
    }
    return create_input(device, size, size, DXGI_FORMAT_R32G32_FLOAT, vectors.data(), size * 8);
}

struct Mask {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11RenderTargetView> view;
};

std::optional<Mask> make_mask(ID3D11Device* device) {
    auto description = texture_description(size, size, DXGI_FORMAT_R8_UNORM, D3D11_BIND_RENDER_TARGET);
    Mask mask;
    if (FAILED(device->CreateTexture2D(&description, nullptr, &mask.texture))) return std::nullopt;
    if (FAILED(device->CreateRenderTargetView(mask.texture.Get(), nullptr, &mask.view))) return std::nullopt;
    return mask;
}

std::optional<Image> read_mask(const TestDevice& gpu, ID3D11Texture2D* texture) {
    auto description = texture_description(size, size, DXGI_FORMAT_R8_UNORM, 0);
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(gpu.device->CreateTexture2D(&description, nullptr, &staging))) return std::nullopt;
    gpu.context->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(gpu.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return std::nullopt;
    Image values(size * size);
    for (UINT row = 0; row < size; ++row)
        std::memcpy(values.data() + row * size, static_cast<const unsigned char*>(mapped.pData) + row * mapped.RowPitch, size);
    gpu.context->Unmap(staging.Get(), 0);
    return values;
}

std::optional<Image> run(const TestDevice& gpu, ReactiveMaskPass& pass, const Image& current, const Image& previous,
    float motion_x) {
    auto mask = make_mask(gpu.device.Get());
    auto now = color_view(gpu.device.Get(), current);
    auto before = color_view(gpu.device.Get(), previous);
    auto motion = motion_view(gpu.device.Get(), motion_x, 0.0f);
    if (!mask || !now || !before || !motion) return std::nullopt;
    pass.render(gpu.context.Get(), {now.Get(), before.Get(), motion.Get(), mask->view.Get()}, {size, size});
    return read_mask(gpu, mask->texture.Get());
}

int new_object_case(const TestDevice& gpu, ReactiveMaskPass& pass) {
    auto previous = scene(edge);
    auto current = previous;
    paint_block(current, 2, 6, 4);
    auto mask = run(gpu, pass, current, previous, 0.0f);
    if (!mask) return 10;
    if ((*mask)[7 * size + 3] < 250) return 11;
    if ((*mask)[1 * size + 1] != 0 || (*mask)[12 * size + 13] != 0) return 12;
    return (*mask)[7 * size + 7] == 0 ? 0 : 13;
}

int moved_scene_case(const TestDevice& gpu, ReactiveMaskPass& pass) {
    auto previous = scene(edge);
    auto current = scene(edge - 3);
    auto right = run(gpu, pass, current, previous, 3.0f);
    if (!right) return 20;
    for (std::uint8_t value : *right)
        if (value != 0) return 21;
    auto wrong = run(gpu, pass, current, previous, 0.0f);
    if (!wrong) return 22;
    return (*wrong)[4 * size + 5] > 0 ? 0 : 23;
}
}

int main() {
    auto gpu = create_test_device();
    if (!gpu) return 1;
    ReactiveMaskPass pass;
    if (FAILED(pass.initialize(gpu->device.Get(), module_directory() / L"NeuralFX" / L"shaders"))) return 2;
    for (auto test_case : {new_object_case, moved_scene_case}) {
        if (int code = test_case(*gpu, pass); code != 0) {
            std::cerr << "reactive_mask case failed: " << code << '\n';
            return code;
        }
    }
    pass.render(gpu->context.Get(), {nullptr, nullptr, nullptr, nullptr}, {size, size});
    return 0;
}
