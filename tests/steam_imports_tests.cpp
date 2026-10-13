#include "screenshots/steam_imports.hpp"

#include <windows.h>
#include <array>
#include <cstring>

namespace {
template <typename T>
void put(std::span<std::byte> image, std::size_t offset, const T& value) {
    std::memcpy(image.data() + offset, &value, sizeof(value));
}
void put_text(std::span<std::byte> image, std::size_t offset, const char* value) {
    std::memcpy(image.data() + offset, value, std::strlen(value) + 1);
}
}

int main() {
    std::array<std::byte, 2048> image{};
    IMAGE_DOS_HEADER dos{};
    dos.e_magic = IMAGE_DOS_SIGNATURE;
    dos.e_lfanew = 64;
    put(image, 0, dos);
    IMAGE_NT_HEADERS64 nt{};
    nt.Signature = IMAGE_NT_SIGNATURE;
    nt.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    nt.OptionalHeader.SizeOfImage = static_cast<DWORD>(image.size());
    nt.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] = {400, 40};
    put(image, 64, nt);
    IMAGE_IMPORT_DESCRIPTOR descriptor{};
    descriptor.Name = 600;
    descriptor.OriginalFirstThunk = 700;
    put(image, 400, descriptor);
    put_text(image, 600, "steam_api64.dll");
    IMAGE_THUNK_DATA64 thunk{};
    thunk.u1.AddressOfData = 800;
    put(image, 700, thunk);
    put_text(image, 802, "SteamAPI_RunCallbacks");
    if (!neuralfx::uses_standard_steam_dispatch(image)) return 1;
    thunk.u1.AddressOfData = 900;
    put(image, 708, thunk);
    put_text(image, 902, "SteamAPI_ManualDispatch_RunFrame");
    if (neuralfx::uses_standard_steam_dispatch(image)) return 2;
    put_text(image, 902, "SteamAPI_GetHSteamUser");
    if (!neuralfx::uses_standard_steam_dispatch(image)) return 3;
    put_text(image, 600, "other.dll");
    if (neuralfx::uses_standard_steam_dispatch(image)) return 4;
    put_text(image, 600, "steam_api64.dll");
    descriptor.OriginalFirstThunk = 2047;
    put(image, 400, descriptor);
    if (neuralfx::uses_standard_steam_dispatch(image)) return 5;
    if (neuralfx::uses_standard_steam_dispatch(std::span(image).first(32))) return 6;
    dos.e_lfanew = -1;
    put(image, 0, dos);
    if (neuralfx::uses_standard_steam_dispatch(image)) return 7;
    dos.e_lfanew = 64;
    put(image, 0, dos);
    descriptor.OriginalFirstThunk = 700;
    descriptor.FirstThunk = 1200;
    put(image, 400, descriptor);
    thunk.u1.AddressOfData = 900;
    put(image, 708, thunk);
    put_text(image, 902, "SteamAPI_RegisterCallback");
    thunk.u1.AddressOfData = 1000;
    put(image, 716, thunk);
    put_text(image, 1002, "SteamAPI_UnregisterCallback");
    auto imports = neuralfx::steam_callback_imports(image);
    if (!imports || imports->registration != 1208 || imports->unregistration != 1216) return 8;
    put_text(image, 1002, "SteamAPI_ManualDispatch_RunFrame");
    if (neuralfx::steam_callback_imports(image)) return 9;
    put_text(image, 1002, "SteamAPI_UnregisterCallback");
    descriptor.FirstThunk = 2040;
    put(image, 400, descriptor);
    if (neuralfx::steam_callback_imports(image)) return 10;
    descriptor.FirstThunk = 1201;
    put(image, 400, descriptor);
    if (neuralfx::steam_callback_imports(image)) return 11;
    return 0;
}
