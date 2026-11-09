#include "diagnostics/trace_trigger.hpp"

#include <fstream>

using namespace neuralfx::diagnostics;
using namespace std::chrono_literals;

namespace {
std::filesystem::path scratch() {
    auto directory = std::filesystem::temp_directory_path() / "neuralfx_trace_trigger_tests";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    return directory;
}

void touch(const std::filesystem::path& path) {
    std::ofstream file(path);
}
}

int main() {
    auto request = scratch() / "capture-trace";
    TraceTrigger trigger(request);
    auto start = std::chrono::steady_clock::time_point{} + 10s;
    if (trigger.poll(start) != TriggerResult::Waiting) return 1;
    touch(request);
    if (trigger.poll(start + 500ms) != TriggerResult::Waiting) return 2;
    if (!std::filesystem::exists(request)) return 3;
    if (trigger.poll(start + 1s) != TriggerResult::Requested) return 4;
    if (std::filesystem::exists(request)) return 5;
    if (trigger.poll(start + 2s) != TriggerResult::Waiting) return 6;
    std::filesystem::create_directories(request / "blocked");
    if (trigger.poll(start + 3s) != TriggerResult::Failed) return 7;
    std::filesystem::remove_all(request.parent_path());
    return 0;
}
