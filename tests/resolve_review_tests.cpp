#include "resolve_values.hpp"
#include "temporal/resolve_review.hpp"

#include <cmath>
#include <iostream>
#include <limits>

using namespace neuralfx::temporal;
using namespace resolve_test;

namespace {
constexpr RenderSize size{1440, 1080};

FrameFloats read(std::uint64_t frame, const Values& values) {
    return {frame, std::vector<float>(values.begin(), values.end())};
}

int mixed_case() {
    Values matched = game_values();
    jitter_by(matched, 0.0585f, 0.1619f);
    Values off_sequence = game_values();
    jitter_by(off_sequence, 0.3f, 0.3f);
    Values mismatched = game_values(1920.0f, 1080.0f);
    std::vector<FrameFloats> reads{read(5, matched), read(6, off_sequence), read(7, mismatched)};
    reads.push_back({8, std::vector<float>(10)});
    auto report = review_resolves(reads, size);
    if (report.samples.size() != 2 || report.rejected.size() != 2) return 10;
    if (report.samples[0].frame != 5 || report.samples[0].phase != 11u) return 11;
    if (report.samples[1].frame != 6 || report.samples[1].phase) return 12;
    if (report.rejected[0].frame != 7 || report.rejected[0].fault != ResolveFault::SizeMismatch) return 13;
    return report.rejected[1].fault == ResolveFault::TooShort ? 0 : 14;
}

int non_finite_case() {
    Values broken = game_values();
    broken[33] = std::numeric_limits<float>::infinity();
    std::vector<FrameFloats> reads{read(9, broken)};
    auto report = review_resolves(reads, size);
    if (report.rejected.size() != 1 || report.rejected[0].fault != ResolveFault::NotFinite) return 40;
    if (report.rejected[0].index != 33u) return 41;
    return std::isinf(report.rejected[0].value) ? 0 : 42;
}

int notable_case() {
    ResolveSample still{1, ResolveFrame{{0.0585f, 0.1619f}, 0.001f, 0.01f}, 11u};
    if (is_notable(still)) return 20;
    ResolveSample moving = still;
    moving.values.move_m = 0.3f;
    if (!is_notable(moving)) return 21;
    ResolveSample turning = still;
    turning.values.turn_deg = 0.6f;
    if (!is_notable(turning)) return 22;
    ResolveSample off_sequence = still;
    off_sequence.phase.reset();
    return is_notable(off_sequence) ? 0 : 23;
}

int empty_case() {
    auto report = review_resolves({}, size);
    return report.samples.empty() && report.rejected.empty() ? 0 : 30;
}
}

int main() {
    for (auto test_case : {mixed_case, non_finite_case, notable_case, empty_case}) {
        if (int code = test_case(); code != 0) {
            std::cerr << "resolve_review case failed: " << code << '\n';
            return code;
        }
    }
    return 0;
}
