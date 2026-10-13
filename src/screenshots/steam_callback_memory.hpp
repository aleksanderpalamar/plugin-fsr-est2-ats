#pragma once

#include "screenshots/steam_callback.hpp"

namespace neuralfx {
inline std::uint8_t steam_callback_flags(const SteamCallback* callback) noexcept {
    std::uint8_t flags = 0;
    std::memcpy(&flags, reinterpret_cast<const std::byte*>(callback) + 8, sizeof(flags));
    return flags;
}

inline void steam_callback_registration(SteamCallback* callback, int id, bool registered) noexcept {
    auto flags = steam_callback_flags(callback);
    flags = static_cast<std::uint8_t>((flags & ~1u) | (registered ? 1u : 0u));
    std::memcpy(reinterpret_cast<std::byte*>(callback) + 8, &flags, sizeof(flags));
    std::memcpy(reinterpret_cast<std::byte*>(callback) + 12, &id, sizeof(id));
}

inline const SteamCallbackFunctions* steam_callback_table(const SteamCallback* callback) noexcept {
    const SteamCallbackFunctions* table = nullptr;
    std::memcpy(&table, callback, sizeof(table));
    return table;
}
}
