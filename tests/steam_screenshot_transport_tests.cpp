#include "screenshots/steam_transport.hpp"

namespace {
using namespace neuralfx;
bool initialized = true;
bool hooked = false;
bool hook_failure = false;
bool registration_failure = false;
int hook_calls = 0;
int registrations = 0;
int unregistrations = 0;
int writes = 0;
int handle = 12;
SteamCallback* request = nullptr;
SteamCallback* ready = nullptr;
int user() { return initialized ? 1 : 0; }
void* screenshots() { return initialized ? &hooked : nullptr; }
void hook(void*, bool enable) { ++hook_calls; hooked = enable && !hook_failure; }
bool is_hooked(void*) { return hooked; }
void register_callback(SteamCallback* callback, int id) {
    ++registrations;
    callback->flags = registration_failure ? 0 : 1;
    callback->callback = id;
    if (id == 2302) request = callback;
    if (id == 2301) ready = callback;
}
void unregister_callback(SteamCallback* callback) { ++unregistrations; callback->flags = 0; }
std::uint32_t write(void* self, void* rgb, std::uint32_t size, int width, int height) {
    ++writes;
    if (self != &hooked || size != 3 || width != 1 || height != 1) return 0;
    auto* bytes = static_cast<std::uint8_t*>(rgb);
    if (!bytes || bytes[0] != 1 || bytes[1] != 2 || bytes[2] != 3) return 0;
    return handle;
}
struct Events final : ScreenshotEvents {
    int requests = 0;
    int completions = 0;
    int result = 0;
    void requested() noexcept override { ++requests; }
    void ready(std::uint32_t, int value) noexcept override { ++completions; result = value; }
};
}

int main() {
    SteamScreenshotFunctions api{user, user, screenshots, register_callback,
        unregister_callback, hook, is_hooked, write, true};
    SteamScreenshotTransport transport(api);
    Events events;
    if (!transport.available()) return 1;
    initialized = false;
    if (transport.available() || transport.activate(events) != ScreenshotActivation::Unavailable) return 2;
    initialized = true;
    api.standard_dispatch = false;
    if (transport.available()) return 3;
    api.standard_dispatch = true;
    api.unregister_callback = nullptr;
    if (transport.available()) return 4;
    api.unregister_callback = unregister_callback;
    hooked = true;
    if (transport.activate(events) != ScreenshotActivation::Conflict || registrations != 0) return 5;
    if (hook_calls != 0) return 6;
    hooked = false;
    registration_failure = true;
    if (transport.activate(events) != ScreenshotActivation::Unavailable || hook_calls != 0) return 7;
    registration_failure = false;
    hook_failure = true;
    if (transport.activate(events) != ScreenshotActivation::Unavailable || hooked) return 8;
    hook_failure = false;
    if (transport.activate(events) != ScreenshotActivation::Active || !transport.active()) return 9;
    if (!request || !ready || request->size() != 1 || ready->size() != 8) return 10;
    request->run(nullptr);
    request->run(nullptr, false, 0);
    request->run(nullptr, true, 0);
    if (events.requests != 2 || writes != 0) return 11;
    RgbImage image{1, 1, {1, 2, 3}};
    if (transport.write(image) != 12 || writes != 1) return 12;
    SteamScreenshotReady completion{12, 1};
    ready->run(&completion);
    completion.result = 37;
    ready->run(&completion);
    if (events.completions != 2 || events.result != 37) return 13;
    image.pixels.pop_back();
    if (transport.write(image) || writes != 1) return 14;
    handle = 0;
    image.pixels.push_back(3);
    if (transport.write(image)) return 15;
    transport.deactivate();
    if (hooked || transport.active() || request->registered() || ready->registered()) return 16;
    request->run(nullptr);
    if (events.requests != 2 || unregistrations == 0) return 17;
    return 0;
}
