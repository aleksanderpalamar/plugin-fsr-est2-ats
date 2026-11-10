#pragma once

#include <cstdint>
#include <vector>

namespace neuralfx::temporal {
struct FrameFloats {
    std::uint64_t frame = 0;
    std::vector<float> values;
};
}
