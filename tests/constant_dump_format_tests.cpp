#include "diagnostics/constant_dump_format.hpp"

#include <iostream>

using namespace neuralfx;

namespace {
int expect(const std::string& line, const std::string& wanted, int code) {
    if (line == wanted) return 0;
    std::cerr << "case " << code << " produced: " << line << '\n';
    return code;
}

int chunk_case() {
    diagnostics::ConstantSnapshot snapshot{1, 0xab, 256, 160, {}, diagnostics::ShaderStage::Pixel};
    for (int index = 0; index < 40; ++index) snapshot.values.push_back(static_cast<float>(index) * 0.5f);
    auto lines = diagnostics::constant_lines(12, temporal::PassRole::TemporalResolve, snapshot);
    if (lines.size() != 2) return 10;
    std::string first = "Pass constants frame=12 pass=resolve stage=ps slot=1 buffer=0xab offset=256 size=160 first=0 values=0";
    if (lines[0].rfind(first, 0) != 0) return expect(lines[0], first, 11);
    return expect(lines[1], "Pass constants frame=12 pass=resolve stage=ps slot=1 buffer=0xab offset=256 size=160 "
        "first=32 values=16,16.5,17,17.5,18,18.5,19,19.5", 12);
}

int empty_case() {
    diagnostics::ConstantSnapshot snapshot{0, 0x10, 0, 128, {}, diagnostics::ShaderStage::Vertex};
    auto lines = diagnostics::constant_lines(3, temporal::PassRole::SceneLighting, snapshot);
    if (lines.size() != 1) return 20;
    return expect(lines[0],
        "Pass constants frame=3 pass=lighting stage=vs slot=0 buffer=0x10 offset=0 size=128 values=unknown", 21);
}
}

int main() {
    for (auto test_case : {chunk_case, empty_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
