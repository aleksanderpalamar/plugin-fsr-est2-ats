#pragma once

#include <d3d11.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace neuralfx::upscale {
struct Readback {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> bytes;
};

std::optional<Readback> read_texture(ID3D11DeviceContext* context, ID3D11Texture2D* texture);
std::optional<std::vector<std::uint8_t>> read_buffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer);
}
