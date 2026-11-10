#pragma once

#include "temporal/frame_floats.hpp"
#include "temporal/resolve_constants.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace neuralfx::temporal {
struct ResolveSample {
    std::uint64_t frame = 0;
    ResolveFrame values;
    std::optional<std::uint32_t> phase;
};

struct RejectedResolve {
    std::uint64_t frame = 0;
    ResolveFault fault = ResolveFault::TooShort;
    std::optional<std::size_t> index = std::nullopt;
    float value = 0.0f;
};

struct ResolveReport {
    std::vector<ResolveSample> samples;
    std::vector<RejectedResolve> rejected;
};

ResolveReport review_resolves(std::span<const FrameFloats> reads, RenderSize size);
bool is_notable(const ResolveSample& sample) noexcept;
}
