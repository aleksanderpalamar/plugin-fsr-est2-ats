#pragma once

namespace neuralfx {
struct alignas(16) ScreenSpaceConstants {
    float inverse_width;
    float inverse_height;
    float strength;
    float frame_index;
};

static_assert(sizeof(ScreenSpaceConstants) == 16);
}
