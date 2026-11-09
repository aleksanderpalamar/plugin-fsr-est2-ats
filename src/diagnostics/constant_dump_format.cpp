#include "diagnostics/constant_dump_format.hpp"
#include "diagnostics/number_format.hpp"

#include <algorithm>
#include <array>
#include <charconv>

namespace neuralfx::diagnostics {
namespace {
constexpr std::size_t values_per_line = 32;

std::string hex(std::uint64_t value) {
    std::array<char, 24> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
    return "0x" + std::string(buffer.data(), result.ptr);
}

const char* pass_name(temporal::PassRole role) {
    switch (role) {
    case temporal::PassRole::SceneLighting: return "lighting";
    case temporal::PassRole::TemporalResolve: return "resolve";
    case temporal::PassRole::SceneGeometry: return "geometry";
    case temporal::PassRole::Output: return "output";
    case temporal::PassRole::Other: return "other";
    }
    return "unknown";
}

std::string header(std::uint64_t frame, temporal::PassRole role, const ConstantSnapshot& snapshot) {
    return "Pass constants frame=" + std::to_string(frame) + " pass=" + pass_name(role) +
        " stage=" + (snapshot.stage == ShaderStage::Pixel ? "ps" : "vs") + " slot=" + std::to_string(snapshot.slot) +
        " buffer=" + hex(snapshot.buffer) + " offset=" + std::to_string(snapshot.offset) +
        " size=" + std::to_string(snapshot.size);
}

std::string chunk(const std::vector<float>& values, std::size_t first) {
    std::string text;
    auto last = std::min(values.size(), first + values_per_line);
    for (auto index = first; index < last; ++index) text += (index > first ? "," : "") + decimal(values[index]);
    return text;
}
}

std::vector<std::string> constant_lines(std::uint64_t frame, temporal::PassRole role,
    const ConstantSnapshot& snapshot) {
    auto prefix = header(frame, role, snapshot);
    if (snapshot.values.empty()) return {prefix + " values=unknown"};
    std::vector<std::string> lines;
    for (std::size_t first = 0; first < snapshot.values.size(); first += values_per_line)
        lines.push_back(prefix + " first=" + std::to_string(first) + " values=" + chunk(snapshot.values, first));
    return lines;
}
}
