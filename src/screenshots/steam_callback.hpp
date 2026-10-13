#pragma once

#include "screenshots/interfaces.hpp"

#include <atomic>
#include <cstddef>
#include <cstring>

namespace neuralfx {
struct SteamCallback;
struct SteamCallbackFunctions {
    void (*extended)(SteamCallback*, void*, bool, std::uint64_t);
    void (*plain)(SteamCallback*, void*);
    int (*size)(SteamCallback*);
};

struct SteamCallback {
public:
    SteamCallback(const SteamCallbackFunctions& functions, int id) : table(&functions), callback(id) {}
    void run(void* data) noexcept { table->plain(this, data); }
    void run(void* data, bool failed, std::uint64_t call) noexcept { table->extended(this, data, failed, call); }
    int size() noexcept { return table->size(this); }
    bool registered() const noexcept { return (flags & 1) != 0; }
    const SteamCallbackFunctions* table;
    std::uint8_t flags = 0;
    int callback;
};

struct SteamScreenshotReady {
    std::uint32_t handle;
    int result;
};

class SteamScreenshotCallback final : public SteamCallback {
public:
    explicit SteamScreenshotCallback(int id) : SteamCallback(functions_, id) {}
    void listen(ScreenshotEvents* events) noexcept { events_.store(events); }

private:
    void dispatch(void* data) noexcept {
        auto* events = events_.load();
        if (!events) return;
        if (callback == 2302) {
            events->requested();
            return;
        }
        if (!data) return;
        SteamScreenshotReady ready{};
        std::memcpy(&ready, data, sizeof(ready));
        events->ready(ready.handle, ready.result);
    }
    static void plain(SteamCallback* base, void* data) noexcept {
        static_cast<SteamScreenshotCallback*>(base)->dispatch(data);
    }
    static void extended(SteamCallback* base, void* data, bool failed, std::uint64_t) noexcept {
        if (!failed) plain(base, data);
    }
    static int buffer_size(SteamCallback* base) noexcept {
        return base->callback == 2302 ? 1 : static_cast<int>(sizeof(SteamScreenshotReady));
    }
    inline static constexpr SteamCallbackFunctions functions_{extended, plain, buffer_size};
    std::atomic<ScreenshotEvents*> events_{nullptr};
};

static_assert(sizeof(SteamCallback) == 16);
static_assert(offsetof(SteamCallback, flags) == 8);
static_assert(offsetof(SteamCallback, callback) == 12);
static_assert(sizeof(SteamCallbackFunctions) == 24);
static_assert(sizeof(SteamScreenshotReady) == 8);
}
