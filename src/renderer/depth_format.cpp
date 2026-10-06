#include "renderer/depth_format.hpp"

namespace neuralfx {
std::optional<DepthFormat> depth_format(DXGI_FORMAT source) noexcept {
    switch (source) {
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_TYPELESS:
        return DepthFormat{DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM};
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
        return DepthFormat{DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS};
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_TYPELESS:
        return DepthFormat{DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT};
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return DepthFormat{DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS};
    default:
        return std::nullopt;
    }
}
}
