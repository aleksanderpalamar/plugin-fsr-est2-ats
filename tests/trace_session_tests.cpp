#include "diagnostics/trace_session.hpp"

#include <array>
#include <string>

using namespace neuralfx::diagnostics;

namespace {
constexpr TraceTarget backbuffer{0x100, 1920, 1080, 28, 1};
constexpr TraceTarget scene{0x200, 1920, 1080, 10, 1};

void game_frame(TraceSession& session, int draws) {
    if (!session.recording()) return;
    std::array targets{scene};
    session.trace().bind(targets, std::nullopt);
    for (int index = 0; index < draws; ++index) session.trace().draw();
}

bool ends_with_draws(const std::vector<std::string>& lines, int draws) {
    if (lines.empty()) return false;
    return lines.back() == "Trace frame end entries=1 draws=" + std::to_string(draws) + " dropped=0";
}

int idle_case() {
    TraceSession session(64);
    if (!session.present_begin().empty()) return 1;
    session.present_end(backbuffer);
    if (session.recording()) return 2;
    return session.state() == SessionState::Idle ? 0 : 3;
}

int capture_case() {
    TraceSession session(64);
    if (!session.request(2)) return 10;
    if (session.state() != SessionState::Armed) return 11;
    if (!session.present_begin().empty()) return 12;
    session.present_end(backbuffer);
    if (!session.recording()) return 13;
    game_frame(session, 3);
    auto first = session.present_begin();
    if (!ends_with_draws(first, 3)) return 14;
    if (session.state() != SessionState::Paused) return 15;
    game_frame(session, 9);
    session.present_end(backbuffer);
    game_frame(session, 5);
    if (!ends_with_draws(session.present_begin(), 5)) return 16;
    if (session.state() != SessionState::Idle) return 17;
    session.present_end(backbuffer);
    return session.recording() ? 18 : 0;
}

int state_reset_case() {
    TraceSession session(64);
    session.pipeline().set_pixel_shader(0x55);
    if (!session.request(1)) return 30;
    return session.pipeline().pixel_shader() == 0 ? 0 : 31;
}

int busy_case() {
    TraceSession session(64);
    if (session.request(0)) return 20;
    if (!session.request(1)) return 21;
    if (session.request(5)) return 22;
    session.present_end(backbuffer);
    if (session.request(5)) return 23;
    game_frame(session, 1);
    if (!ends_with_draws(session.present_begin(), 1)) return 24;
    return session.state() == SessionState::Idle ? 0 : 25;
}
}

int main() {
    for (auto test_case : {idle_case, capture_case, state_reset_case, busy_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
