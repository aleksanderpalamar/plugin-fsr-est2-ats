#pragma once

#include <dxgiformat.h>
#include <optional>

namespace neuralfx {
struct DepthFormat {
    DXGI_FORMAT texture;
    DXGI_FORMAT view;
};

std::optional<DepthFormat> depth_format(DXGI_FORMAT source) noexcept;
}
