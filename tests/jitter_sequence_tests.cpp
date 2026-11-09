#include "temporal/jitter_sequence.hpp"

#include <array>
#include <cmath>
#include <iostream>

using namespace neuralfx::temporal;

namespace {
constexpr std::array<Jitter, jitter_period> observed{{
    {-0.24512373f, 0.4301597f}, {-0.49025512f, -0.13968062f}, {0.26460576f, 0.29047918f},
    {0.019501671f, -0.27936125f}, {-0.22563192f, 0.15079832f}, {-0.47076553f, -0.41904187f},
    {0.28412488f, 0.011117935f}, {0.038996276f, 0.44127753f}, {-0.206115f, -0.12856291f},
    {-0.451214f, 0.30159664f}, {0.30364656f, -0.2682433f}, {0.0585376f, 0.16191626f},
    {-0.18656471f, -0.40792415f}, {-0.43172565f, 0.02223587f}, {0.32319185f, 0.45239544f},
    {0.07805837f, -0.117444985f}}};

int captured_sequence_case() {
    for (std::uint32_t phase = 0; phase < jitter_period; ++phase) {
        auto jitter = sequence_jitter(phase);
        if (std::fabs(jitter.x - observed[phase].x) > 2e-4f || std::fabs(jitter.y - observed[phase].y) > 2e-4f) {
            std::cerr << "phase " << phase << ": " << jitter.x << ',' << jitter.y << '\n';
            return 10;
        }
    }
    return 0;
}

int wraps_case() {
    auto first = sequence_jitter(3);
    auto wrapped = sequence_jitter(3 + jitter_period * 5);
    return first.x == wrapped.x && first.y == wrapped.y ? 0 : 20;
}

int phase_case() {
    for (std::uint32_t phase = 0; phase < jitter_period; ++phase) {
        auto found = sequence_phase(observed[phase]);
        if (!found || *found != phase) return 30;
    }
    if (sequence_phase(Jitter{0.0f, 0.0f})) return 31;
    if (sequence_phase(Jitter{observed[4].x + 0.01f, observed[4].y})) return 32;
    return 0;
}
}

int main() {
    for (auto test_case : {captured_sequence_case, wraps_case, phase_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
