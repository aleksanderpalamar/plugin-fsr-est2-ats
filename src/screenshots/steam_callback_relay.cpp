#include "screenshots/steam_callback_relay.hpp"
#include "screenshots/steam_callback_memory.hpp"

namespace neuralfx {
SteamCallbackRelay::SteamCallbackRelay() : SteamCallback(functions_, 2302) {}

bool SteamCallbackRelay::attach(SteamCallback* original, SteamCallbackRegistration api,
    SteamCallbackRouting& routing) {
    std::lock_guard lock(mutex_);
    if (original_.load() == original) return true;
    if (original_.load()) return false;
    routing_ = &routing;
    original_.store(original);
    flags = 0;
    api.add(this, 2302);
    steam_callback_registration(original, 2302, registered());
    if (registered()) return true;
    original_.store(nullptr);
    return false;
}

bool SteamCallbackRelay::release(SteamCallback* original, SteamCallbackRegistration api) {
    std::lock_guard lock(mutex_);
    if (original_.load() != original) return false;
    if (routing_->leader.load() == this) routing_->events.store(nullptr);
    if (registered()) api.remove(this);
    steam_callback_registration(original, 2302, false);
    original_.store(nullptr);
    return true;
}

bool SteamCallbackRelay::matches(SteamCallback* original) const noexcept { return original_.load() == original; }
bool SteamCallbackRelay::occupied() const noexcept { return original_.load() != nullptr; }
bool SteamCallbackRelay::activate(ScreenshotEvents& events) {
    std::lock_guard lock(mutex_);
    if (!occupied() || !registered()) return false;
    routing_->leader.store(this);
    routing_->events.store(&events);
    return true;
}

void SteamCallbackRelay::dispatch(void* data, bool extended_call, bool failed, std::uint64_t call) {
    std::lock_guard lock(mutex_);
    auto* original = original_.load();
    if (!original) return;
    auto* events = routing_->events.load();
    if (!events) {
        forward(original, data, extended_call, failed, call);
        return;
    }
    if (routing_->leader.load() != this || failed) return;
    events->requested();
}

void SteamCallbackRelay::forward(SteamCallback* original, void* data, bool extended_call,
    bool failed, std::uint64_t call) {
    auto* table = steam_callback_table(original);
    if (extended_call) {
        table->extended(original, data, failed, call);
        return;
    }
    table->plain(original, data);
}

void SteamCallbackRelay::plain(SteamCallback* base, void* data) {
    static_cast<SteamCallbackRelay*>(base)->dispatch(data, false, false, 0);
}
void SteamCallbackRelay::extended(SteamCallback* base, void* data, bool failed, std::uint64_t call) {
    static_cast<SteamCallbackRelay*>(base)->dispatch(data, true, failed, call);
}
int SteamCallbackRelay::buffer_size(SteamCallback* base) {
    auto* relay = static_cast<SteamCallbackRelay*>(base);
    std::lock_guard lock(relay->mutex_);
    auto* original = relay->original_.load();
    return original ? steam_callback_table(original)->size(original) : 1;
}
}
