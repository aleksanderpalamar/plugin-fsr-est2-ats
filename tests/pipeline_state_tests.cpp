#include "diagnostics/pipeline_state.hpp"

#include <array>
#include <cstdint>
#include <vector>

using namespace neuralfx::diagnostics;

namespace {
int resource_case() {
    PipelineState state;
    std::array<std::uint64_t, 3> first{0x10, 0, 0x30};
    state.set_resources(2, first);
    if (state.inputs() != std::vector<std::uint64_t>{0x10, 0x30}) return 1;
    std::array<std::uint64_t, 1> replace{0x40};
    state.set_resources(4, replace);
    if (state.inputs() != std::vector<std::uint64_t>{0x10, 0x40}) return 2;
    std::array<std::uint64_t, 1> outside{0x50};
    state.set_resources(200, outside);
    if (state.inputs() != std::vector<std::uint64_t>{0x10, 0x40}) return 3;
    state.set_pixel_shader(0xabc);
    return state.pixel_shader() == 0xabc ? 0 : 4;
}

int reset_case() {
    PipelineState state;
    std::array<std::uint64_t, 1> ids{0x10};
    state.set_resources(0, ids);
    state.set_pixel_shader(0x20);
    state.reset();
    return state.inputs().empty() && state.pixel_shader() == 0 ? 0 : 10;
}
}

int main() {
    for (auto test_case : {resource_case, reset_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
