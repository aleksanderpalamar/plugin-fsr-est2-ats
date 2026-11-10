#pragma once

#include <cstdint>

namespace neuralfx::temporal {
struct StageTimes {
    std::int64_t capture_us = 0;
    std::int64_t camera_collect_us = 0;
    std::int64_t resolve_collect_us = 0;
};
}
