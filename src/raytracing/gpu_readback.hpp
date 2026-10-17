#pragma once

#include "raytracing/image.hpp"

#include <d3d11.h>
#include <optional>

namespace neuralfx::raytracing {
std::optional<Image> read_image(ID3D11Device* device, ID3D11DeviceContext* context,
    ID3D11Texture2D* texture, int width, int height);
}
