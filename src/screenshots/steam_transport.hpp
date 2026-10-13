#pragma once

#include "screenshots/steam_callback.hpp"
#include "screenshots/steam_callback_relay.hpp"

namespace neuralfx {
struct SteamScreenshotFunctions {
    int (*user)() = nullptr;
    int (*pipe)() = nullptr;
    void* (*screenshots)() = nullptr;
    void (*register_callback)(SteamCallback*, int) = nullptr;
    void (*unregister_callback)(SteamCallback*) = nullptr;
    void (*hook)(void*, bool) = nullptr;
    bool (*hooked)(void*) = nullptr;
    std::uint32_t (*write)(void*, void*, std::uint32_t, int, int) = nullptr;
    bool standard_dispatch = false;
};

class SteamScreenshotTransport final : public ScreenshotTransport {
public:
    explicit SteamScreenshotTransport(SteamScreenshotFunctions& functions,
        SteamScreenshotRelaySource* relays = nullptr) : api_(functions), relays_(relays) {}
    bool available() noexcept override;
    ScreenshotActivation activate(ScreenshotEvents& events) noexcept override;
    bool active() noexcept override;
    void deactivate() noexcept override;
    std::uint32_t write(const RgbImage& image) noexcept override;

private:
    void* current_interface() noexcept;
    void unregister() noexcept;
    void unregister(SteamCallback& callback) noexcept;
    ScreenshotActivation activate_game(void* screenshots, ScreenshotEvents& events) noexcept;
    enum class Owner { None, Plugin, Game };
    SteamScreenshotFunctions& api_;
    SteamScreenshotRelaySource* relays_;
    SteamScreenshotCallback requested_{2302};
    SteamScreenshotCallback ready_{2301};
    void* owned_ = nullptr;
    Owner owner_ = Owner::None;
};
}
