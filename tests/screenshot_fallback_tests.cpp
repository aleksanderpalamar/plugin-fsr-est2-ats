#include "screenshot_review_fixture.hpp"
#include "screenshots/coordinator.hpp"

#include <cstdio>
#include <stdexcept>

namespace {
using namespace screenshot_test;
enum class Failure { Read, Write, Exception, Frame };
struct Observer final : ScreenshotObserver {
    ScreenshotEvent last = ScreenshotEvent::Activated;
    int count = 0;
    void report(ScreenshotEvent event, std::uint32_t, int value) noexcept override { last = event; count = value; }
};
struct Reader final : ScreenshotReader {
    Failure failure;
    explicit Reader(Failure value) : failure(value) {}
    bool prepare() override { return true; }
    std::optional<RgbImage> capture() override {
        if (failure == Failure::Read) return std::nullopt;
        if (failure == Failure::Exception) throw std::runtime_error("readback");
        return RgbImage{1, 1, {12, 34, 56}};
    }
};

int fallback(Failure failure, int requests = 1, bool game_handler = true) {
    SteamFixture steam;
    Events game_events;
    SteamScreenshotCallback game{2302};
    game.listen(&game_events);
    if (game_handler) steam.relays.register_callback(&game, 2302);
    steam.hooked = game_handler;
    steam.fail_write = failure == Failure::Write;
    SteamScreenshotTransport transport(steam.functions, &steam.relays);
    Observer observer;
    Reader reader(failure);
    ScreenshotCoordinator coordinator(transport, observer);
    int chain = 0;
    coordinator.present(&chain, FrameStatus::Processed, reader);
    for (int index = 0; index < requests; ++index) steam.press();
    steam.pump();
    if (game_events.requests != 0) return 1;
    auto frame = failure == Failure::Frame ? FrameStatus::Failed : FrameStatus::Processed;
    coordinator.present(&chain, frame, reader);
    std::fprintf(stderr, "capture failure: retriggers=%d pending=%d game=%d\n",
        steam.retriggers, steam.pending, game_events.requests);
    if (steam.pending != requests || steam.retriggers != requests) return 2;
    if (game_events.requests != 0 || transport.active()) return 3;
    if (observer.last != ScreenshotEvent::FallbackRequested || observer.count != requests) return 7;
    steam.pump();
    if (game_handler && game_events.requests != requests) return 4;
    if (!game_handler && steam.automatic != requests) return 4;
    coordinator.present(&chain, FrameStatus::Processed, reader);
    if (steam.pending != 0 || steam.retriggers != requests) return 5;
    if (game_handler) steam.relays.unregister_callback(&game);
    return steam.callbacks.empty() ? 0 : 6;
}

int unavailable() {
    SteamFixture steam;
    SteamScreenshotTransport transport(steam.functions, &steam.relays);
    Observer observer;
    Reader reader(Failure::Read);
    ScreenshotCoordinator coordinator(transport, observer);
    int chain = 0;
    coordinator.present(&chain, FrameStatus::Processed, reader);
    steam.press();
    steam.pump();
    steam.initialized = false;
    coordinator.present(&chain, FrameStatus::Processed, reader);
    if (steam.retriggers != 0) return 8;
    return observer.last == ScreenshotEvent::FallbackFailed && observer.count == 1 ? 0 : 9;
}
}

int main() {
    for (auto failure : {Failure::Read, Failure::Write, Failure::Exception, Failure::Frame}) {
        int result = fallback(failure);
        if (result != 0) return result;
        result = fallback(failure, 1, false);
        if (result != 0) return result;
    }
    int result = fallback(Failure::Read, 3);
    if (result != 0) return result;
    return unavailable();
}
