#include "temporal/resolve_review.hpp"
#include "temporal/jitter_sequence.hpp"

namespace neuralfx::temporal {
namespace {
RejectedResolve rejection(const FrameFloats& read, ResolveFault fault) {
    RejectedResolve rejected{read.frame, fault, std::nullopt, 0.0f};
    if (fault != ResolveFault::NotFinite) return rejected;
    rejected.index = first_non_finite(read.values);
    if (rejected.index) rejected.value = read.values[*rejected.index];
    return rejected;
}

constexpr float notable_move_m = 0.05f;
constexpr float notable_turn_deg = 0.5f;
}

ResolveReport review_resolves(std::span<const FrameFloats> reads, RenderSize size) {
    ResolveReport report;
    for (const auto& read : reads) {
        auto result = evaluate_resolve(read.values, size);
        if (const auto* fault = std::get_if<ResolveFault>(&result)) {
            report.rejected.push_back(rejection(read, *fault));
            continue;
        }
        const auto& values = std::get<ResolveFrame>(result);
        report.samples.push_back({read.frame, values, sequence_phase(values.jitter)});
    }
    return report;
}

bool is_notable(const ResolveSample& sample) noexcept {
    return !sample.phase || sample.values.move_m >= notable_move_m || sample.values.turn_deg >= notable_turn_deg;
}
}
