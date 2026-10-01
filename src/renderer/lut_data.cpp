#include "renderer/lut_data.hpp"

#include <cmath>
#include <sstream>
#include <string>

namespace neuralfx {
namespace {
bool parse_domain(std::istringstream& line, float expected) {
    float red = 0;
    float green = 0;
    float blue = 0;
    return static_cast<bool>(line >> red >> green >> blue)
        && red == expected && green == expected && blue == expected;
}

bool parse_range(std::istringstream& line) {
    float minimum = 0;
    float maximum = 0;
    return static_cast<bool>(line >> minimum >> maximum)
        && minimum == 0.0f && maximum == 1.0f;
}

bool append_texel(LutData& data, std::istringstream& line) {
    if (data.size < 2) return false;
    std::array<float, 4> texel{0.0f, 0.0f, 0.0f, 1.0f};
    if (!(line >> texel[0] >> texel[1] >> texel[2])) return false;
    std::string extra;
    if (line >> extra && extra[0] != '#') return false;
    for (unsigned index = 0; index < 3; ++index) {
        if (!std::isfinite(texel[index])) return false;
    }
    if (data.texels.size() >= data.size * data.size * data.size) return false;
    data.texels.push_back(texel);
    return true;
}

bool parse_size(LutData& data, std::istringstream& line) {
    unsigned size = 0;
    if (data.size != 0 || !(line >> size) || size < 2 || size > 64) return false;
    data.size = size;
    data.texels.reserve(size * size * size);
    return true;
}

bool consume_line(LutData& data, const std::string& text) {
    std::istringstream line(text);
    std::string token;
    if (!(line >> token) || token[0] == '#') return true;
    if (token == "TITLE") return true;
    if (token == "LUT_3D_SIZE") return parse_size(data, line);
    if (token == "DOMAIN_MIN") return parse_domain(line, 0.0f);
    if (token == "DOMAIN_MAX") return parse_domain(line, 1.0f);
    if (token == "LUT_3D_INPUT_RANGE") return parse_range(line);
    if (token == "LUT_1D_SIZE") return false;
    line.clear();
    line.str(text);
    return append_texel(data, line);
}
}

std::optional<LutData> parse_lut(std::istream& input) {
    LutData data;
    std::string text;
    while (std::getline(input, text)) {
        if (!consume_line(data, text)) return std::nullopt;
    }
    if (data.size < 2 || data.texels.size() != data.size * data.size * data.size) return std::nullopt;
    return data;
}
}
