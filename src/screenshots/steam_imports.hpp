#pragma once

#include <cstddef>
#include <span>
#include <optional>

namespace neuralfx {
bool uses_standard_steam_dispatch(std::span<const std::byte> image) noexcept;
struct SteamCallbackImports {
    std::size_t registration;
    std::size_t unregistration;
};
std::optional<SteamCallbackImports> steam_callback_imports(std::span<const std::byte> image) noexcept;
}
