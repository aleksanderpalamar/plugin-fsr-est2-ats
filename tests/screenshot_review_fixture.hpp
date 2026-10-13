#pragma once

#include "screenshots/steam_transport.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace screenshot_test {
using namespace neuralfx;

class SteamFixture {
public:
    SteamFixture() { current_ = this; functions.trigger = trigger; }
    SteamScreenshotFunctions functions{user, user, screenshots, add, remove, hook, is_hooked, write, true};
    SteamCallbackRelays relays{{add, remove}};
    bool hooked = false;
    bool initialized = true;
    bool fail_write = false;
    int writes = 0;
    int retriggers = 0;
    int automatic = 0;
    int pending = 0;
    std::vector<SteamCallback*> callbacks;

    void press() { ++pending; }
    void pump(bool reverse = false) {
        int count = std::exchange(pending, 0);
        for (int index = 0; index < count; ++index) dispatch(reverse);
    }

    static void trigger(void*) { ++current_->retriggers; current_->press(); }

private:
    void dispatch(bool reverse) {
        if (!hooked) { ++automatic; return; }
        auto registered = callbacks;
        if (reverse) std::reverse(registered.begin(), registered.end());
        for (auto* callback : registered)
            if (callback->registered() && callback->callback == 2302) callback->run(nullptr);
    }
    static int user() { return current_->initialized ? 1 : 0; }
    static void* screenshots() { return current_->initialized ? current_ : nullptr; }
    static void hook(void*, bool enable) { current_->hooked = enable; }
    static bool is_hooked(void*) { return current_->hooked; }
    static void add(SteamCallback* callback, int id) {
        callback->callback = id;
        callback->flags |= 1;
        current_->callbacks.push_back(callback);
    }
    static void remove(SteamCallback* callback) {
        callback->flags &= ~1;
        std::erase(current_->callbacks, callback);
    }
    static std::uint32_t write(void*, void*, std::uint32_t, int, int) {
        ++current_->writes;
        return current_->fail_write ? 0 : static_cast<std::uint32_t>(current_->writes);
    }
    inline static SteamFixture* current_ = nullptr;
};

struct Events final : ScreenshotEvents {
    int requests = 0;
    void requested() noexcept override { ++requests; }
    void ready(std::uint32_t, int) noexcept override {}
};
}
