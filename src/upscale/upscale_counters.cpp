#include "upscale/upscale_counters.hpp"

namespace neuralfx::upscale {
namespace {
constexpr std::uint64_t summary_interval = 600;

std::string field(const char* name, std::uint64_t value) {
    return std::string(" ") + name + "=" + std::to_string(value);
}
}

std::string counters_line(const UpscaleCounters& counters) {
    return "Upscale summary" + field("outputs", counters.outputs) + field("replaced", counters.replaced) +
        field("upscaled", counters.upscaled) + field("contract_invalid", counters.contract_invalid) +
        field("backend_invalid", counters.backend_invalid) + field("failed", counters.failed) +
        field("not_replaced", counters.not_replaced);
}

bool summary_due(std::uint64_t outputs) noexcept {
    return outputs > 0 && outputs % summary_interval == 0;
}
}
