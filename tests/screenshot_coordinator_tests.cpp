#include "screenshots/coordinator.hpp"

#include <stdexcept>
#include <array>
#include <thread>
#include <vector>

namespace {
using namespace neuralfx;
struct Observer final : ScreenshotObserver {
    std::vector<ScreenshotEvent> events;
    void report(ScreenshotEvent event, std::uint32_t, int) noexcept override {
        events.push_back(event);
    }
};
struct Reader final : ScreenshotReader {
    int preparations = 0;
    int reads = 0;
    bool supported = true;
    bool failed = false;
    bool throws = false;
    bool prepare() override { ++preparations; return supported; }
    std::optional<RgbImage> capture() override {
        ++reads;
        if (throws) throw std::runtime_error("readback");
        if (failed) return std::nullopt;
        return RgbImage{1, 1, {12, 34, 56}};
    }
};
struct Transport final : ScreenshotTransport {
    ScreenshotEvents* callbacks = nullptr;
    ScreenshotActivation activation = ScreenshotActivation::Active;
    bool present = true;
    bool hooked = false;
    bool fail_write = false;
    int writes = 0;
    int activations = 0;
    int stops = 0;
    bool available() noexcept override { return present; }
    ScreenshotActivation activate(ScreenshotEvents& events) noexcept override {
        ++activations;
        callbacks = &events;
        hooked = activation == ScreenshotActivation::Active;
        return activation;
    }
    bool active() noexcept override { return present && hooked; }
    void deactivate() noexcept override { ++stops; hooked = false; }
    std::uint32_t write(const RgbImage& image) noexcept override {
        ++writes;
        if (image.pixels != std::vector<std::uint8_t>{12, 34, 56}) return 0;
        return fail_write ? 0 : static_cast<std::uint32_t>(writes);
    }
};
struct Fixture {
    Observer observer;
    Reader reader;
    Transport transport;
    ScreenshotCoordinator coordinator{transport, observer};
    int primary = 0;
    int secondary = 0;
    void frame(FrameStatus status = FrameStatus::Processed) {
        coordinator.present(&primary, status, reader);
    }
};
}

int main() {
    Fixture normal;
    normal.frame();
    if (!normal.transport.hooked || normal.reader.reads != 0) return 1;
    normal.coordinator.requested();
    if (normal.reader.reads != 0) return 2;
    normal.frame(FrameStatus::Skipped);
    normal.coordinator.present(&normal.secondary, FrameStatus::Processed, normal.reader);
    if (normal.reader.reads != 0) return 3;
    normal.frame();
    normal.frame();
    if (normal.reader.reads != 1 || normal.transport.writes != 1) return 4;
    normal.coordinator.requested();
    normal.frame(FrameStatus::Original);
    if (normal.transport.writes != 2) return 5;
    normal.coordinator.ready(2, 1);
    normal.coordinator.ready(3, 37);
    if (normal.observer.events.back() != ScreenshotEvent::ReadyFailed) return 6;
    if (normal.coordinator.reset(&normal.secondary)) return 7;
    normal.coordinator.requested();
    if (!normal.coordinator.reset(&normal.primary)) return 8;
    normal.frame();
    if (normal.transport.writes != 3) return 9;

    Fixture absent;
    absent.transport.present = false;
    absent.frame();
    if (absent.transport.activations != 0 || absent.reader.preparations != 0) return 10;
    absent.transport.present = true;
    absent.frame();
    if (!absent.transport.hooked) return 11;
    Fixture unsupported;
    unsupported.reader.supported = false;
    unsupported.frame();
    if (unsupported.transport.activations != 0) return 12;
    Fixture conflict;
    conflict.transport.activation = ScreenshotActivation::Conflict;
    conflict.frame();
    conflict.frame();
    if (conflict.transport.activations != 1 || conflict.transport.stops != 0) return 13;

    Fixture failed;
    failed.frame();
    failed.reader.failed = true;
    failed.coordinator.requested();
    failed.frame();
    failed.frame();
    if (failed.transport.hooked || failed.reader.reads != 1 || failed.transport.writes != 0) return 14;
    Fixture write_failure;
    write_failure.frame();
    write_failure.transport.fail_write = true;
    write_failure.coordinator.requested();
    write_failure.frame();
    write_failure.frame();
    if (write_failure.transport.hooked || write_failure.transport.writes != 1) return 15;
    Fixture unfinished;
    unfinished.frame();
    unfinished.coordinator.requested();
    unfinished.frame(FrameStatus::Failed);
    if (unfinished.transport.hooked || unfinished.reader.reads != 0) return 16;
    Fixture exception;
    exception.frame();
    exception.reader.throws = true;
    exception.coordinator.requested();
    exception.frame();
    if (exception.transport.hooked) return 17;
    Fixture multiple;
    multiple.frame();
    multiple.coordinator.requested();
    multiple.coordinator.requested();
    multiple.frame();
    multiple.frame();
    multiple.frame();
    if (multiple.transport.writes != 2) return 18;
    Fixture threaded;
    threaded.frame();
    std::array<std::thread, 4> callbacks;
    for (auto& callback : callbacks) callback = std::thread([&] {
        for (int index = 0; index < 8; ++index) threaded.coordinator.requested();
    });
    for (auto& callback : callbacks) callback.join();
    for (int index = 0; index < 40; ++index) threaded.frame();
    if (threaded.transport.writes != 32) return 19;
    Fixture recreated;
    recreated.frame();
    recreated.coordinator.requested();
    recreated.coordinator.reset(nullptr);
    recreated.coordinator.present(&recreated.secondary, FrameStatus::Processed, recreated.reader);
    recreated.frame();
    if (recreated.transport.writes != 1) return 20;
    return 0;
}
