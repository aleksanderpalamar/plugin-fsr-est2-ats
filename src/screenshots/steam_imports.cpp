#include "screenshots/steam_imports.hpp"

#include <windows.h>
#include <cstring>
#include <optional>
#include <string_view>

namespace neuralfx {
namespace {
template <typename T>
std::optional<T> read(std::span<const std::byte> image, std::size_t offset) noexcept {
    if (offset > image.size() || sizeof(T) > image.size() - offset) return std::nullopt;
    T value{};
    std::memcpy(&value, image.data() + offset, sizeof(value));
    return value;
}

std::optional<std::string_view> text(std::span<const std::byte> image, std::size_t offset) noexcept {
    if (offset >= image.size()) return std::nullopt;
    auto* start = reinterpret_cast<const char*>(image.data() + offset);
    auto* end = static_cast<const char*>(std::memchr(start, 0, image.size() - offset));
    if (!end) return std::nullopt;
    return std::string_view(start, static_cast<std::size_t>(end - start));
}

struct Dispatch {
    bool standard = false;
    bool manual = false;
    std::size_t registration = 0;
    std::size_t unregistration = 0;
};

std::optional<Dispatch> scan_thunks(std::span<const std::byte> image,
    const IMAGE_IMPORT_DESCRIPTOR& descriptor) noexcept {
    auto start = descriptor.OriginalFirstThunk;
    if (start == 0) return std::nullopt;
    Dispatch dispatch;
    for (std::size_t offset = start; offset < image.size(); offset += sizeof(IMAGE_THUNK_DATA64)) {
        auto thunk = read<IMAGE_THUNK_DATA64>(image, offset);
        if (!thunk) return std::nullopt;
        if (thunk->u1.AddressOfData == 0) return dispatch;
        if (IMAGE_SNAP_BY_ORDINAL64(thunk->u1.Ordinal)) continue;
        if (thunk->u1.AddressOfData >= image.size()) return std::nullopt;
        auto name = text(image, static_cast<std::size_t>(thunk->u1.AddressOfData) + sizeof(WORD));
        if (!name) return std::nullopt;
        dispatch.standard |= *name == "SteamAPI_RunCallbacks";
        dispatch.manual |= name->starts_with("SteamAPI_ManualDispatch_");
        auto slot = static_cast<std::size_t>(descriptor.FirstThunk) + offset - start;
        if (slot % alignof(std::uint64_t) != 0 || !read<std::uint64_t>(image, slot)) continue;
        if (*name == "SteamAPI_RegisterCallback") dispatch.registration = slot;
        if (*name == "SteamAPI_UnregisterCallback") dispatch.unregistration = slot;
    }
    return std::nullopt;
}
std::optional<Dispatch> scan_imports(std::span<const std::byte> image) noexcept {
    auto dos = read<IMAGE_DOS_HEADER>(image, 0);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0) return std::nullopt;
    auto nt = read<IMAGE_NT_HEADERS64>(image, static_cast<std::size_t>(dos->e_lfanew));
    if (!nt || nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return std::nullopt;
    if (nt->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_IMPORT) return std::nullopt;
    if (nt->OptionalHeader.SizeOfImage == 0 || nt->OptionalHeader.SizeOfImage > image.size()) return std::nullopt;
    image = image.first(nt->OptionalHeader.SizeOfImage);
    auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (directory.VirtualAddress == 0 || directory.VirtualAddress >= image.size()) return std::nullopt;
    if (directory.Size > image.size() - directory.VirtualAddress) return std::nullopt;
    auto end = static_cast<std::size_t>(directory.VirtualAddress) + directory.Size;
    Dispatch found;
    for (std::size_t offset = directory.VirtualAddress;
        offset + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= end; offset += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        auto descriptor = read<IMAGE_IMPORT_DESCRIPTOR>(image, offset);
        if (!descriptor) return std::nullopt;
        if (descriptor->Name == 0) return found;
        auto library = text(image, descriptor->Name);
        if (!library) return std::nullopt;
        if (_stricmp(library->data(), "steam_api64.dll") != 0) continue;
        auto dispatch = scan_thunks(image, *descriptor);
        if (!dispatch) return std::nullopt;
        found.standard |= dispatch->standard;
        found.manual |= dispatch->manual;
        if (dispatch->registration) found.registration = dispatch->registration;
        if (dispatch->unregistration) found.unregistration = dispatch->unregistration;
    }
    return std::nullopt;
}
}

bool uses_standard_steam_dispatch(std::span<const std::byte> image) noexcept {
    auto found = scan_imports(image);
    return found && found->standard && !found->manual;
}

std::optional<SteamCallbackImports> steam_callback_imports(std::span<const std::byte> image) noexcept {
    auto found = scan_imports(image);
    if (!found || !found->standard || found->manual) return std::nullopt;
    if (!found->registration || !found->unregistration) return std::nullopt;
    return SteamCallbackImports{found->registration, found->unregistration};
}
}
