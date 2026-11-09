#pragma once

#include "diagnostics/frame_trace.hpp"
#include "temporal/scene_recognizer.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace neuralfx::diagnostics {
std::vector<std::string> constant_lines(std::uint64_t frame, temporal::PassRole role,
    const ConstantSnapshot& snapshot);
}
