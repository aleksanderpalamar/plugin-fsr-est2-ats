#pragma once

#include "temporal/jitter_sequence.hpp"

#include <cstdint>
#include <optional>

namespace neuralfx::temporal {
enum class SyncResult { Confirmed, Resynced, Lost };

struct JitterObservation {
    std::uint64_t frame = 0;
    Jitter jitter;
};

class JitterTracker {
public:
    SyncResult observe(const JitterObservation& observation) noexcept;
    std::optional<Jitter> predict(std::uint64_t frame) const noexcept;
    void reset() noexcept { anchor_.reset(); }

private:
    struct Anchor {
        std::uint64_t frame = 0;
        std::uint32_t phase = 0;
    };

    std::optional<std::uint32_t> phase_at(std::uint64_t frame) const noexcept;

    std::optional<Anchor> anchor_;
};
}
