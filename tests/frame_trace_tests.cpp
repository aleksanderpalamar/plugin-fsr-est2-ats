#include "diagnostics/frame_trace.hpp"

#include <array>
#include <iostream>
#include <string>
#include <vector>

using namespace neuralfx::diagnostics;

namespace {
constexpr TraceTarget backbuffer{0x100, 1920, 1080, 28, 1};
constexpr TraceTarget scene{0x200, 1280, 720, 10, 1};
constexpr TraceTarget depth{0x300, 1280, 720, 40, 1};
constexpr TraceTarget motion{0x400, 1280, 720, 34, 1};

int expect(const std::vector<std::string>& lines, const std::vector<std::string>& expected, int code) {
    if (lines == expected) return 0;
    std::cerr << "case " << code << " produced:\n";
    for (const auto& line : lines) std::cerr << line << '\n';
    return code;
}

int composition_case() {
    FrameTrace trace(16);
    trace.begin(backbuffer);
    trace.clear_color(scene.id);
    std::array scene_targets{scene, motion};
    trace.bind(scene_targets, depth);
    trace.viewport({1280.0f, 720.0f});
    trace.draw();
    trace.draw();
    trace.read(0x999);
    std::array output{backbuffer};
    trace.bind(output, std::nullopt);
    trace.viewport({1920.0f, 1080.0f});
    trace.read(scene.id);
    trace.read(scene.id);
    trace.draw();
    trace.draw();
    return expect(trace.lines(), {
        "Trace frame begin backbuffer=bb 1920x1080 fmt=28 samples=1",
        "Trace 0 clear-color rt=200",
        "Trace 1 pass rt=200 rt=400 ds=300 vp=1280x720 draws=2",
        "Trace 2 pass rt=bb vp=1920x1080 draws=2 reads=200",
        "Trace target bb 1920x1080 fmt=28 samples=1 color writes=1 reads=0",
        "Trace target 200 1280x720 fmt=10 samples=1 color writes=1 reads=1",
        "Trace target 300 1280x720 fmt=40 samples=1 depth writes=1 reads=0",
        "Trace target 400 1280x720 fmt=34 samples=1 color writes=1 reads=0",
        "Trace frame end entries=3 draws=4 dropped=0"}, 1);
}

int inherited_binding_case() {
    FrameTrace trace(16);
    trace.begin(backbuffer);
    trace.draw();
    return expect(trace.lines(), {
        "Trace frame begin backbuffer=bb 1920x1080 fmt=28 samples=1",
        "Trace 0 pass inherited draws=1",
        "Trace target bb 1920x1080 fmt=28 samples=1 color writes=0 reads=0",
        "Trace frame end entries=1 draws=1 dropped=0"}, 2);
}

int copy_case() {
    FrameTrace trace(16);
    trace.begin(backbuffer);
    std::array targets{scene};
    trace.bind(targets, std::nullopt);
    trace.draw();
    constexpr TraceTarget history{0x500, 1280, 720, 10, 1};
    trace.copy(scene.id, history);
    trace.copy(0x777, TraceTarget{0x778, 64, 64, 28, 1});
    std::array output{backbuffer};
    trace.bind(output, std::nullopt);
    trace.read(history.id);
    trace.draw();
    return expect(trace.lines(), {
        "Trace frame begin backbuffer=bb 1920x1080 fmt=28 samples=1",
        "Trace 0 pass rt=200 draws=1",
        "Trace 1 copy src=200 dst=500",
        "Trace 2 pass rt=bb draws=1 reads=500",
        "Trace target bb 1920x1080 fmt=28 samples=1 color writes=1 reads=0",
        "Trace target 200 1280x720 fmt=10 samples=1 color writes=1 reads=1",
        "Trace target 500 1280x720 fmt=10 samples=1 color writes=1 reads=1",
        "Trace frame end entries=3 draws=2 dropped=0"}, 3);
}

int limit_case() {
    FrameTrace trace(2);
    trace.begin(backbuffer);
    std::array targets{scene};
    for (int index = 0; index < 4; ++index) {
        trace.bind(targets, std::nullopt);
        trace.draw();
    }
    return expect(trace.lines(), {
        "Trace frame begin backbuffer=bb 1920x1080 fmt=28 samples=1",
        "Trace 0 pass rt=200 draws=1",
        "Trace 1 pass rt=200 draws=1",
        "Trace target bb 1920x1080 fmt=28 samples=1 color writes=0 reads=0",
        "Trace target 200 1280x720 fmt=10 samples=1 color writes=2 reads=0",
        "Trace frame end entries=2 draws=4 dropped=2"}, 4);
}

int restart_case() {
    FrameTrace trace(16);
    trace.begin(backbuffer);
    std::array targets{scene};
    trace.bind(targets, std::nullopt);
    trace.draw();
    trace.begin(backbuffer);
    return expect(trace.lines(), {
        "Trace frame begin backbuffer=bb 1920x1080 fmt=28 samples=1",
        "Trace target bb 1920x1080 fmt=28 samples=1 color writes=0 reads=0",
        "Trace frame end entries=0 draws=0 dropped=0"}, 5);
}
}

int main() {
    for (auto test_case : {composition_case, inherited_binding_case, copy_case, limit_case, restart_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
