#include "renderer/depth_format.hpp"

int main() {
    using neuralfx::depth_format;
    auto depth24 = depth_format(DXGI_FORMAT_D24_UNORM_S8_UINT);
    if (!depth24 || depth24->texture != DXGI_FORMAT_R24G8_TYPELESS) return 1;
    if (depth24->view != DXGI_FORMAT_R24_UNORM_X8_TYPELESS) return 2;
    auto depth32 = depth_format(DXGI_FORMAT_D32_FLOAT);
    if (!depth32 || depth32->texture != DXGI_FORMAT_R32_TYPELESS) return 3;
    if (depth32->view != DXGI_FORMAT_R32_FLOAT) return 4;
    if (depth_format(DXGI_FORMAT_R8G8B8A8_UNORM)) return 5;
    return 0;
}
