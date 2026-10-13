#pragma once

#include "screenshots/steam_transport.hpp"

#include <windows.h>
#include <chrono>

namespace neuralfx {
class SteamScreenshotLoader {
public:
    void poll() noexcept;
    SteamScreenshotFunctions& functions() noexcept { return functions_; }

private:
    enum class State { Waiting, Missing, Ready, Unsupported };
    State state_ = State::Waiting;
    HMODULE module_ = nullptr;
    SteamScreenshotFunctions functions_;
    std::chrono::steady_clock::time_point next_poll_{};
};
}
