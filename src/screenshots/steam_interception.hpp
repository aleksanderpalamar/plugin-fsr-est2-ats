#pragma once

#include "screenshots/steam_callback_relay.hpp"

#include <windows.h>

namespace neuralfx {
void install_steam_callback_interception() noexcept;
SteamScreenshotRelaySource* game_steam_screenshot_relays();
void log_steam_callback_interception() noexcept;
}
