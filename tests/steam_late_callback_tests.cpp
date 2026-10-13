#include "screenshot_review_fixture.hpp"

#include <cstdio>

namespace {
using namespace screenshot_test;

int late_callback(bool reverse) {
    SteamFixture steam;
    Events game_events;
    Events plugin_events;
    SteamScreenshotCallback game{2302};
    game.listen(&game_events);
    SteamScreenshotTransport transport(steam.functions, &steam.relays);
    if (transport.activate(plugin_events) != ScreenshotActivation::Active) return 1;
    steam.relays.register_callback(&game, 2302);
    steam.press();
    steam.pump(reverse);
    std::fprintf(stderr, "late handler: plugin=%d game=%d\n", plugin_events.requests, game_events.requests);
    if (plugin_events.requests != 1 || game_events.requests != 0) return 2;
    transport.deactivate();
    if (!steam.hooked) return 3;
    steam.press();
    steam.pump(reverse);
    if (plugin_events.requests != 1 || game_events.requests != 1) return 4;
    steam.relays.unregister_callback(&game);
    return steam.callbacks.empty() ? 0 : 5;
}

int removed_leader() {
    SteamFixture steam;
    Events game_events;
    Events plugin_events;
    SteamScreenshotCallback first{2302};
    SteamScreenshotCallback second{2302};
    first.listen(&game_events);
    second.listen(&game_events);
    SteamScreenshotTransport transport(steam.functions, &steam.relays);
    if (transport.activate(plugin_events) != ScreenshotActivation::Active) return 6;
    steam.relays.register_callback(&first, 2302);
    steam.relays.register_callback(&second, 2302);
    steam.relays.unregister_callback(&first);
    steam.press();
    steam.pump();
    if (plugin_events.requests != 1 || game_events.requests != 0) return 7;
    transport.deactivate();
    if (!steam.hooked) return 8;
    steam.press();
    steam.pump();
    if (plugin_events.requests != 1 || game_events.requests != 1) return 10;
    steam.relays.unregister_callback(&second);
    return steam.callbacks.empty() ? 0 : 9;
}
}

int main() {
    int result = late_callback(false);
    if (result != 0) return result;
    result = late_callback(true);
    if (result != 0) return result;
    return removed_leader();
}
