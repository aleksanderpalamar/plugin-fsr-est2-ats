#include "screenshots/steam_callback.hpp"

#include <cstring>

namespace {
struct Events final : neuralfx::ScreenshotEvents {
    int requests = 0;
    int completions = 0;
    void requested() noexcept override { ++requests; }
    void ready(std::uint32_t handle, int result) noexcept override {
        if (handle == 42 && result == 1) ++completions;
    }
};

template <typename Function>
Function native_slot(neuralfx::SteamCallback* callback, int index) {
    const std::byte* table = nullptr;
    std::memcpy(&table, callback, sizeof(table));
    Function function = nullptr;
    std::memcpy(&function, table + static_cast<std::size_t>(index) * sizeof(function), sizeof(function));
    return function;
}
}

int main() {
    using Callback = neuralfx::SteamCallback;
    using Extended = void(*)(Callback*, void*, bool, std::uint64_t);
    using Plain = void(*)(Callback*, void*);
    using Size = int(*)(Callback*);
    Events events;
    neuralfx::SteamScreenshotCallback requested(2302);
    requested.listen(&events);
    native_slot<Extended>(&requested, 0)(&requested, nullptr, true, 0);
    if (events.requests != 0) return 1;
    native_slot<Plain>(&requested, 1)(&requested, nullptr);
    if (events.requests != 1) return 2;
    if (native_slot<Size>(&requested, 2)(&requested) != 1) return 3;
    neuralfx::SteamScreenshotCallback ready(2301);
    ready.listen(&events);
    neuralfx::SteamScreenshotReady result{42, 1};
    native_slot<Plain>(&ready, 1)(&ready, &result);
    native_slot<Extended>(&ready, 0)(&ready, &result, false, 99);
    if (events.completions != 2) return 4;
    if (native_slot<Size>(&ready, 2)(&ready) != 8) return 5;
    return 0;
}
