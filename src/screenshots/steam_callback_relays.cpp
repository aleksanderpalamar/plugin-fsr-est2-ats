#include "screenshots/steam_callback_relay.hpp"
#include "screenshots/steam_callback_memory.hpp"

namespace neuralfx {
SteamCallbackRelay* SteamCallbackRelays::find(SteamCallback* callback) noexcept {
    for (auto& relay : relays_)
        if (relay.matches(callback)) return &relay;
    return nullptr;
}

void SteamCallbackRelays::register_callback(SteamCallback* callback, int id) {
    if (!callback || id != 2302 || (steam_callback_flags(callback) & 2) != 0) {
        api_.add(callback, id);
        return;
    }
    if (find(callback)) return;
    for (auto& relay : relays_) {
        if (relay.occupied()) continue;
        if (relay.attach(callback, api_, routing_)) return;
    }
    complete_.store(false);
    deactivate();
    api_.add(callback, id);
}

void SteamCallbackRelays::unregister_callback(SteamCallback* callback) {
    if (!callback) return api_.remove(callback);
    auto* relay = find(callback);
    if (relay && relay->release(callback, api_)) return;
    api_.remove(callback);
}

bool SteamCallbackRelays::activate(ScreenshotEvents& events) noexcept {
    if (!complete_.load()) return false;
    for (auto& relay : relays_)
        if (relay.activate(events)) return true;
    return false;
}

bool SteamCallbackRelays::active() const noexcept { return routing_.events.load() != nullptr; }
void SteamCallbackRelays::deactivate() noexcept { routing_.events.store(nullptr); }
}
