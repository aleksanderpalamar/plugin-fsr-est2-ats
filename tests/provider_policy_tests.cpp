#include "upscale/provider_policy.hpp"

#include <iostream>
#include <vector>

using namespace neuralfx::upscale;

namespace {
ProviderVersion provider(std::uint64_t id, const char* name) {
    return {id, name};
}

const std::vector<ProviderVersion> proton{provider(0xA, "3.1.6"), provider(0xB, "2.3.4")};
const std::vector<ProviderVersion> amd_native{provider(0xC, "4.1.2"), provider(0xA, "3.1.6"), provider(0xB, "2.3.4")};

int version_case() {
    auto parsed = parse_version("4.1.2");
    if (!parsed || parsed->major != 4 || parsed->minor != 1 || parsed->patch != 2) return 10;
    if (parse_version("3.1.6") != FsrVersion{3, 1, 6}) return 11;
    for (const char* text : {"", "4.1", "4.1.2.0", "a.b.c", "4..2", "4.1.-2", "FSR 4.1.2", "4.1.2 "})
        if (parse_version(text)) return 12;
    return 0;
}

int off_case() {
    for (const auto* available : {&proton, &amd_native}) {
        auto choice = select_provider(UpscaleMode::Off, *available);
        if (choice.provider || choice.reason != SelectionReason::Off) return 20;
    }
    return 0;
}

int strict_case() {
    auto native = select_provider(UpscaleMode::Strict, amd_native);
    if (!native.provider || native.provider->id != 0xC || native.reason != SelectionReason::Preferred) return 30;
    auto missing = select_provider(UpscaleMode::Strict, proton);
    if (missing.provider || missing.reason != SelectionReason::PreferredUnavailable) return 31;
    auto empty = select_provider(UpscaleMode::Strict, {});
    return !empty.provider && empty.reason == SelectionReason::PreferredUnavailable ? 0 : 32;
}

int auto_case() {
    auto native = select_provider(UpscaleMode::Auto, amd_native);
    if (!native.provider || native.provider->name != "4.1.2" || native.reason != SelectionReason::Preferred) return 40;
    auto fallback = select_provider(UpscaleMode::Auto, proton);
    if (!fallback.provider || fallback.provider->name != "3.1.6" || fallback.reason != SelectionReason::Fallback)
        return 41;
    auto only_old = select_provider(UpscaleMode::Auto, std::vector{provider(0xB, "2.3.4")});
    if (only_old.provider || only_old.reason != SelectionReason::NoneAvailable) return 42;
    auto empty = select_provider(UpscaleMode::Auto, {});
    return !empty.provider && empty.reason == SelectionReason::NoneAvailable ? 0 : 43;
}

int unparsable_case() {
    auto choice = select_provider(UpscaleMode::Auto, std::vector{provider(1, "unknown"), provider(2, "")});
    return !choice.provider && choice.reason == SelectionReason::NoneAvailable ? 0 : 50;
}

int names_case() {
    if (std::string(reason_name(SelectionReason::Off)) != "off") return 60;
    if (std::string(reason_name(SelectionReason::Preferred)) != "preferred") return 61;
    if (std::string(reason_name(SelectionReason::Fallback)) != "fallback") return 62;
    if (std::string(reason_name(SelectionReason::PreferredUnavailable)) != "preferred-unavailable") return 63;
    return std::string(reason_name(SelectionReason::NoneAvailable)) == "none-available" ? 0 : 64;
}
}

int main() {
    for (auto test_case : {version_case, off_case, strict_case, auto_case, unparsable_case, names_case}) {
        if (int code = test_case(); code != 0) {
            std::cerr << "provider_policy case failed: " << code << '\n';
            return code;
        }
    }
    return 0;
}
