#include "screenshots/steam_transport.hpp"

namespace neuralfx {
void* SteamScreenshotTransport::current_interface() noexcept {
    if (!api_.standard_dispatch || !api_.user || !api_.pipe || !api_.screenshots) return nullptr;
    if (!api_.register_callback || !api_.unregister_callback || !api_.hook || !api_.hooked ||
        !api_.write || !api_.trigger)
        return nullptr;
    if (api_.user() == 0 || api_.pipe() == 0) return nullptr;
    return api_.screenshots();
}

bool SteamScreenshotTransport::available() noexcept { return current_interface() != nullptr; }

ScreenshotActivation SteamScreenshotTransport::activate(ScreenshotEvents& events) noexcept {
    auto* screenshots = current_interface();
    if (!screenshots) return ScreenshotActivation::Unavailable;
    if (owned_ == screenshots && api_.hooked(screenshots))
        return active() ? ScreenshotActivation::Active : ScreenshotActivation::Unavailable;
    if (api_.hooked(screenshots)) return activate_game(screenshots, events);
    events_.store(&events);
    requested_.listen(this);
    ready_.listen(this);
    api_.register_callback(&requested_, 2302);
    api_.register_callback(&ready_, 2301);
    if (!requested_.registered() || !ready_.registered()) {
        unregister();
        return ScreenshotActivation::Unavailable;
    }
    if (relays_ && !relays_->watch()) {
        release_relays();
        unregister();
        return ScreenshotActivation::Unavailable;
    }
    api_.hook(screenshots, true);
    if (!api_.hooked(screenshots)) {
        release_relays();
        api_.hook(screenshots, false);
        unregister();
        return ScreenshotActivation::Unavailable;
    }
    owned_ = screenshots;
    owner_ = Owner::Plugin;
    return ScreenshotActivation::Active;
}

ScreenshotActivation SteamScreenshotTransport::activate_game(void* screenshots,
    ScreenshotEvents& events) noexcept {
    if (!relays_ || !relays_->activate(events)) return ScreenshotActivation::Conflict;
    ready_.listen(&events);
    api_.register_callback(&ready_, 2301);
    if (!ready_.registered()) {
        relays_->deactivate();
        unregister();
        return ScreenshotActivation::Unavailable;
    }
    owned_ = screenshots;
    owner_ = Owner::Game;
    return ScreenshotActivation::Active;
}

bool SteamScreenshotTransport::active() noexcept {
    refresh_owner();
    if (!owned_ || current_interface() != owned_) return false;
    if (owner_ == Owner::Game && !relays_->active()) return false;
    return api_.hooked(owned_);
}

void SteamScreenshotTransport::refresh_owner() noexcept {
    if (owner_ == Owner::Plugin && relays_ && relays_->has_callbacks()) owner_ = Owner::Game;
}

void SteamScreenshotTransport::release_relays() noexcept {
    if (relays_) relays_->deactivate();
}

void SteamScreenshotTransport::requested() noexcept {
    if (relays_ && !relays_->active()) return;
    auto* events = events_.load();
    if (events) events->requested();
}

void SteamScreenshotTransport::ready(std::uint32_t handle, int result) noexcept {
    auto* events = events_.load();
    if (events) events->ready(handle, result);
}

void SteamScreenshotTransport::unregister(SteamCallback& callback) noexcept {
    if (!callback.registered() || !api_.unregister_callback) return;
    api_.unregister_callback(&callback);
}

void SteamScreenshotTransport::unregister() noexcept {
    events_.store(nullptr);
    requested_.listen(nullptr);
    ready_.listen(nullptr);
    unregister(requested_);
    unregister(ready_);
}

void SteamScreenshotTransport::deactivate() noexcept {
    refresh_owner();
    auto* previous = owned_;
    auto owner = owner_;
    owned_ = nullptr;
    owner_ = Owner::None;
    release_relays();
    if (owner == Owner::Plugin && previous && current_interface() == previous && api_.hooked(previous))
        api_.hook(previous, false);
    unregister();
}

std::uint32_t SteamScreenshotTransport::write(const RgbImage& image) noexcept {
    if (!active()) return 0;
    auto size = screenshot_rgb_size(static_cast<std::uint32_t>(image.width),
        static_cast<std::uint32_t>(image.height));
    if (!size || image.pixels.size() != *size) return 0;
    return api_.write(owned_, const_cast<std::uint8_t*>(image.pixels.data()),
        *size, image.width, image.height);
}

bool SteamScreenshotTransport::retry_original(std::uint32_t requests) noexcept {
    if (requests == 0) return true;
    if (owner_ != Owner::None || requested_.registered()) return false;
    auto* screenshots = current_interface();
    if (!screenshots) return false;
    for (std::uint32_t index = 0; index < requests; ++index) api_.trigger(screenshots);
    return true;
}
}
