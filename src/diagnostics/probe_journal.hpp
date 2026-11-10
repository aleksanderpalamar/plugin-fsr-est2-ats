#pragma once

#include "diagnostics/motion_tally.hpp"
#include "temporal/observation_review.hpp"
#include "temporal/resolve_review.hpp"
#include "temporal/temporal_history.hpp"

#include <cstdint>
#include <optional>

namespace neuralfx::diagnostics {
struct FrameFacts {
    std::uint64_t index = 0;
    std::optional<temporal::RenderSize> render;
    std::int64_t cpu_us = 0;
};

class ProbeJournal {
public:
    void reset() noexcept;
    void record(const temporal::TemporalDecision& decision, const FrameFacts& facts,
        const temporal::ObservationReport& report, const temporal::ResolveReport& resolves);

private:
    struct Budget {
        std::uint64_t rejections = 0;
        std::uint64_t jumps = 0;
        std::uint64_t unmatched = 0;
        std::uint64_t resolves = 0;
        std::uint64_t resolve_faults = 0;
    };

    void record_events(const temporal::ObservationReport& report, const temporal::ResolveReport& resolves);

    std::uint64_t frame_ = 0;
    Budget budget_;
    MotionTally tally_;
};
}
