#include "diagnostics/frame_trace.hpp"

#include <array>
#include <iostream>
#include <string>
#include <vector>

using namespace neuralfx::diagnostics;

namespace {
constexpr TraceTarget backbuffer{0x100, 1920, 1080, 28, 1};
constexpr TraceTarget scene{0x200, 1920, 1080, 29, 1};
constexpr TraceTarget depth{0x300, 1920, 1080, 20, 1};
constexpr TraceTarget shadow{0x400, 2048, 2048, 55, 1};

int expect(const std::vector<std::string>& lines, const std::vector<std::string>& expected, int code) {
    if (lines == expected) return 0;
    std::cerr << "case " << code << " produced:\n";
    for (const auto& line : lines) std::cerr << line << '\n';
    return code;
}

int final_pass_case() {
    FrameTrace trace(16);
    trace.begin(backbuffer);
    std::array scene_targets{scene};
    trace.bind(scene_targets, std::nullopt);
    if (trace.wants_draw_detail()) return 10;
    trace.draw();
    std::array output{backbuffer};
    trace.bind(output, std::nullopt);
    if (!trace.wants_draw_detail()) return 11;
    trace.detail({3, 0xa1, {scene.id}});
    trace.draw();
    trace.detail({6, 0xb2, {}});
    trace.draw();
    return expect(trace.lines(), {
        "Trace frame begin backbuffer=bb 1920x1080 fmt=28 samples=1",
        "Trace 0 pass rt=200 draws=1",
        "Trace 1 pass rt=bb draws=2",
        "Trace 2 draw count=3 ps=a1 inputs=200",
        "Trace 3 draw count=6 ps=b2",
        "Trace target bb 1920x1080 fmt=28 samples=1 color writes=1 reads=0",
        "Trace target 200 1920x1080 fmt=29 samples=1 color writes=1 reads=0",
        "Trace frame end entries=4 draws=3 dropped=0"}, 12);
}

int constants_case() {
    FrameTrace trace(16);
    trace.begin(backbuffer);
    trace.bind({}, shadow);
    if (!trace.wants_constants()) return 20;
    trace.draw();
    std::array color_only{scene};
    trace.bind(color_only, std::nullopt);
    if (trace.wants_constants()) return 24;
    trace.draw();
    std::array scene_targets{scene};
    trace.bind(scene_targets, depth);
    if (!trace.wants_constants()) return 21;
    trace.constants({0, 0xc0, 256, 64, {1.0f, -0.5f, 0.25f}});
    trace.constants({2, 0xc2, 0, 32, {}});
    trace.draw();
    if (trace.wants_constants()) return 22;
    return expect(trace.lines(), {
        "Trace frame begin backbuffer=bb 1920x1080 fmt=28 samples=1",
        "Trace 0 pass ds=400 draws=1",
        "Trace 1 pass rt=200 draws=1",
        "Trace 2 pass rt=200 ds=300 draws=1",
        "Trace 3 constants vs slot=0 buffer=c0 offset=256 size=64 values=1,-0.5,0.25",
        "Trace 4 constants vs slot=2 buffer=c2 offset=0 size=32 values=unknown",
        "Trace target bb 1920x1080 fmt=28 samples=1 color writes=0 reads=0",
        "Trace target 200 1920x1080 fmt=29 samples=1 color writes=2 reads=0",
        "Trace target 300 1920x1080 fmt=20 samples=1 depth writes=1 reads=0",
        "Trace target 400 2048x2048 fmt=55 samples=1 depth writes=1 reads=0",
        "Trace frame end entries=5 draws=3 dropped=0"}, 23);
}

int detail_limit_case() {
    FrameTrace trace(64, 2);
    trace.begin(backbuffer);
    std::array output{backbuffer};
    trace.bind(output, std::nullopt);
    for (std::uint32_t index = 0; index < 3; ++index) {
        if (trace.wants_draw_detail()) trace.detail({index, 0, {}});
        trace.draw();
    }
    return expect(trace.lines(), {
        "Trace frame begin backbuffer=bb 1920x1080 fmt=28 samples=1",
        "Trace 0 pass rt=bb draws=3",
        "Trace 1 draw count=0 ps=none",
        "Trace 2 draw count=1 ps=none",
        "Trace target bb 1920x1080 fmt=28 samples=1 color writes=1 reads=0",
        "Trace frame end entries=3 draws=3 dropped=0"}, 30);
}
}

int main() {
    for (auto test_case : {final_pass_case, constants_case, detail_limit_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
