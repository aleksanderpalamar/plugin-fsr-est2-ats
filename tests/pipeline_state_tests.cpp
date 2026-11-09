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

int constants_case() {
    PipelineState state(2);
    std::array<std::uint64_t, 2> buffers{0xc0, 0xc1};
    state.set_constant_buffers(0, buffers);
    std::array<float, 3> values{1.0f, 2.0f, 3.0f};
    state.store_constants(0xc0, values.data(), sizeof(values));
    std::array<float, 4> long_values{4.0f, 5.0f, 6.0f, 7.0f};
    state.store_constants(0xc1, long_values.data(), sizeof(long_values));
    auto snapshots = state.constants(4);
    if (snapshots.size() != 2) return 10;
    if (snapshots[0].slot != 0 || snapshots[0].buffer != 0xc0 || snapshots[0].size != 12) return 11;
    if (snapshots[0].values != std::vector<float>{1.0f, 2.0f}) return 12;
    if (snapshots[1].slot != 1 || snapshots[1].values != std::vector<float>{4.0f, 5.0f}) return 13;
    state.set_constant_buffers(1, std::array<std::uint64_t, 1>{0xc9});
    auto unknown = state.constants(4);
    if (unknown.size() != 2 || unknown[1].buffer != 0xc9 || !unknown[1].values.empty()) return 14;
    state.reset();
    return state.constants(4).empty() && state.inputs().empty() ? 0 : 15;
}
}

int main() {
    for (auto test_case : {resource_case, constants_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
