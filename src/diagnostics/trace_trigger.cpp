#include "diagnostics/trace_trigger.hpp"

#include <system_error>

namespace neuralfx::diagnostics {
TriggerResult TraceTrigger::poll(std::chrono::steady_clock::time_point now) noexcept {
    if (now < next_poll_) return TriggerResult::Waiting;
    next_poll_ = now + std::chrono::seconds(1);
    std::error_code error;
    if (!std::filesystem::exists(request_, error) || error) return TriggerResult::Waiting;
    if (!std::filesystem::remove(request_, error) || error) return TriggerResult::Failed;
    return TriggerResult::Requested;
}
}
