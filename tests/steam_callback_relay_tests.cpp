#include "screenshots/steam_callback_relay.hpp"
#include "screenshots/steam_transport.hpp"

#include <algorithm>
#include <vector>
#include <memory>

namespace {
using namespace neuralfx;
std::vector<SteamCallback*> registered;
bool hooked = true;
bool fail_ready = false;
void add(SteamCallback* callback, int id) {
    callback->callback = id;
    if (fail_ready && id == 2301) return;
    callback->flags |= 1;
    registered.push_back(callback);
}
void remove(SteamCallback* callback) {
    callback->flags &= ~1;
    std::erase(registered, callback);
}
void request(bool extended = false, bool failed = false) {
    auto callbacks = registered;
    for (auto* callback : callbacks) {
        if (callback->callback != 2302 || !callback->registered()) continue;
        if (extended) callback->run(nullptr, failed, 5);
        if (!extended) callback->run(nullptr);
    }
}
int user() { return 1; }
void* screenshots() { return &hooked; }
void hook(void*, bool value) { hooked = value; }
bool is_hooked(void*) { return hooked; }
std::uint32_t write(void*, void*, std::uint32_t, int, int) { return 1; }
struct Events final : ScreenshotEvents {
    int requests = 0;
    void requested() noexcept override { ++requests; }
    void ready(std::uint32_t, int) noexcept override {}
};

int cooperative() {
    SteamCallbackRelays relays({add, remove});
    Events game_events;
    Events plugin_events;
    SteamScreenshotCallback game{2302};
    SteamScreenshotCallback second{2302};
    game.listen(&game_events);
    second.listen(&game_events);
    relays.register_callback(&game, 2302);
    relays.register_callback(&second, 2302);
    relays.register_callback(&game, 2302);
    if (registered.size() != 2 || !game.registered() || !second.registered()) return 1;
    request();
    if (game_events.requests != 2) return 2;
    SteamScreenshotFunctions api{user, user, screenshots, add, remove, hook, is_hooked, write, true};
    SteamScreenshotTransport transport(api, &relays);
    if (transport.activate(plugin_events) != ScreenshotActivation::Active) return 3;
    request();
    request(true, true);
    if (plugin_events.requests != 1 || game_events.requests != 2) return 4;
    transport.deactivate();
    if (!hooked || transport.active()) return 5;
    request(true);
    if (game_events.requests != 4) return 6;
    fail_ready = true;
    if (transport.activate(plugin_events) != ScreenshotActivation::Unavailable) return 7;
    fail_ready = false;
    request();
    if (game_events.requests != 6 || !hooked) return 8;
    if (transport.activate(plugin_events) != ScreenshotActivation::Active) return 9;
    relays.unregister_callback(&game);
    if (transport.active() || game.registered()) return 10;
    request();
    if (game_events.requests != 7 || plugin_events.requests != 1) return 11;
    transport.deactivate();
    relays.unregister_callback(&second);
    if (!registered.empty()) return 12;
    return 0;
}

int overflow() {
    SteamCallbackRelays relays({add, remove});
    Events events;
    std::vector<std::unique_ptr<SteamScreenshotCallback>> callbacks;
    for (int index = 0; index < 33; ++index) {
        auto callback = std::make_unique<SteamScreenshotCallback>(2302);
        callback->listen(&events);
        relays.register_callback(callback.get(), 2302);
        callbacks.push_back(std::move(callback));
    }
    if (relays.activate(events)) return 13;
    request();
    if (events.requests != 33) return 14;
    for (auto& callback : callbacks) relays.unregister_callback(callback.get());
    return registered.empty() ? 0 : 15;
}
}

int main() {
    int result = cooperative();
    if (result != 0) return result;
    return overflow();
}
