#pragma once

#include <windows.h>
#include <bit>

namespace neuralfx {
template <typename Function>
bool patch_vtable(IUnknown* object, size_t index, Function replacement, Function& original) noexcept {
    auto** table = *reinterpret_cast<void***>(object);
    void* address = std::bit_cast<void*>(replacement);
    if (table[index] == address) return true;
    if (original) return false;
    DWORD protection = 0;
    if (!VirtualProtect(&table[index], sizeof(void*), PAGE_READWRITE, &protection)) return false;
    original = std::bit_cast<Function>(table[index]);
    InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(&table[index]), address);
    DWORD unused = 0;
    VirtualProtect(&table[index], sizeof(void*), protection, &unused);
    return true;
}
}
