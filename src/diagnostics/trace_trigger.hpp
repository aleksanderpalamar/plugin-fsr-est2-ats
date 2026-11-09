#pragma once

#include <chrono>
#include <filesystem>

namespace neuralfx::diagnostics {
enum class TriggerResult { Waiting, Requested, Failed };

class TraceTrigger {
public:
    explicit TraceTrigger(std::filesystem::path request) : request_(std::move(request)) {}

    TriggerResult poll(std::chrono::steady_clock::time_point now) noexcept;

private:
    std::filesystem::path request_;
    std::chrono::steady_clock::time_point next_poll_{};
};
}
