#include "upscale/provider_policy.hpp"

#include <algorithm>
#include <charconv>

namespace neuralfx::upscale {
namespace {
constexpr FsrVersion preferred{4, 1, 2};
constexpr FsrVersion fallback{3, 1, 6};

std::optional<int> parse_part(std::string_view text) noexcept {
    int value = 0;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size() || value < 0)
        return std::nullopt;
    return value;
}

const ProviderVersion* find(std::span<const ProviderVersion> available, FsrVersion wanted) {
    auto match = std::find_if(available.begin(), available.end(),
        [&](const ProviderVersion& provider) { return parse_version(provider.name) == wanted; });
    return match == available.end() ? nullptr : &*match;
}

Selection chosen(const ProviderVersion* provider, SelectionReason reason) {
    return {*provider, reason};
}

Selection automatic(std::span<const ProviderVersion> available) {
    if (const auto* best = find(available, preferred)) return chosen(best, SelectionReason::Preferred);
    if (const auto* older = find(available, fallback)) return chosen(older, SelectionReason::Fallback);
    return {std::nullopt, SelectionReason::NoneAvailable};
}
}

std::optional<FsrVersion> parse_version(std::string_view name) noexcept {
    auto first = name.find('.');
    auto second = first == std::string_view::npos ? first : name.find('.', first + 1);
    if (second == std::string_view::npos) return std::nullopt;
    auto major = parse_part(name.substr(0, first));
    auto minor = parse_part(name.substr(first + 1, second - first - 1));
    auto patch = parse_part(name.substr(second + 1));
    if (!major || !minor || !patch) return std::nullopt;
    return FsrVersion{*major, *minor, *patch};
}

Selection select_provider(UpscaleMode mode, std::span<const ProviderVersion> available) {
    if (mode == UpscaleMode::Off) return {std::nullopt, SelectionReason::Off};
    if (mode == UpscaleMode::Auto) return automatic(available);
    if (const auto* best = find(available, preferred)) return chosen(best, SelectionReason::Preferred);
    return {std::nullopt, SelectionReason::PreferredUnavailable};
}

const char* reason_name(SelectionReason reason) noexcept {
    switch (reason) {
    case SelectionReason::Off: return "off";
    case SelectionReason::Preferred: return "preferred";
    case SelectionReason::Fallback: return "fallback";
    case SelectionReason::PreferredUnavailable: return "preferred-unavailable";
    case SelectionReason::NoneAvailable: return "none-available";
    }
    return "unknown";
}
}
