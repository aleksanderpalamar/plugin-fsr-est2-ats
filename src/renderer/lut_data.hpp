#pragma once

#include <array>
#include <iosfwd>
#include <optional>
#include <vector>

namespace neuralfx {
struct LutData {
    unsigned size = 0;
    std::vector<std::array<float, 4>> texels;
};

std::optional<LutData> parse_lut(std::istream& input);
}
