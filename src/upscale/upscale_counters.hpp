#pragma once

#include <cstdint>
#include <string>

namespace neuralfx::upscale {
struct UpscaleCounters {
    std::uint64_t outputs = 0;
    std::uint64_t replaced = 0;
    std::uint64_t upscaled = 0;
    std::uint64_t contract_invalid = 0;
    std::uint64_t backend_invalid = 0;
    std::uint64_t failed = 0;
    std::uint64_t not_replaced = 0;
};

std::string counters_line(const UpscaleCounters& counters);
bool summary_due(std::uint64_t outputs) noexcept;
}
