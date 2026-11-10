#pragma once

#include "upscale/upscale_mode.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace neuralfx::upscale {
struct ProviderVersion {
    std::uint64_t id = 0;
    std::string name;
};

struct FsrVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
    bool operator==(const FsrVersion&) const = default;
};

enum class SelectionReason { Off, Preferred, Fallback, PreferredUnavailable, NoneAvailable };

struct Selection {
    std::optional<ProviderVersion> provider;
    SelectionReason reason = SelectionReason::Off;
};

std::optional<FsrVersion> parse_version(std::string_view name) noexcept;
Selection select_provider(UpscaleMode mode, std::span<const ProviderVersion> available);
const char* reason_name(SelectionReason reason) noexcept;
}
