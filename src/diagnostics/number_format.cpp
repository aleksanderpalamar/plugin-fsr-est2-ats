#include "diagnostics/number_format.hpp"

#include <array>
#include <charconv>

namespace neuralfx::diagnostics {
std::string decimal(float value) {
    std::array<char, 32> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    return std::string(buffer.data(), result.ptr);
}

std::string fixed(float value) {
    std::array<char, 32> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed, 2);
    return std::string(buffer.data(), result.ptr);
}
}
