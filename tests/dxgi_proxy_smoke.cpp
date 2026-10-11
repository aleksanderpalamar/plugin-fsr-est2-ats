#include <windows.h>
#include <dxgi1_2.h>

#include <bit>
#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace {
HMODULE load_system_dxgi() {
    wchar_t path[MAX_PATH]{};
    UINT length = GetSystemDirectoryW(path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH - 9) return nullptr;
    if (wcscat_s(path, L"\\dxgi.dll") != 0) return nullptr;
    HMODULE module = LoadLibraryExW(path, nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    return module ? module : LoadLibraryW(path);
}

const IMAGE_EXPORT_DIRECTORY* export_table(HMODULE module) {
    auto* base = reinterpret_cast<const BYTE*>(module);
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;
    auto location = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (location.VirtualAddress == 0) return nullptr;
    return reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + location.VirtualAddress);
}

bool check_exports(HMODULE system, HMODULE proxy) {
    auto* table = export_table(system);
    if (!table) return false;
    auto* base = reinterpret_cast<const BYTE*>(system);
    auto* names = reinterpret_cast<const DWORD*>(base + table->AddressOfNames);
    auto* functions = reinterpret_cast<const DWORD*>(base + table->AddressOfFunctions);
    for (DWORD index = 0; index < table->NumberOfNames; ++index) {
        auto* name = reinterpret_cast<const char*>(base + names[index]);
        if (GetProcAddress(proxy, name)) continue;
        std::fprintf(stderr, "Missing DXGI export name: %s\n", name);
        return false;
    }
    for (DWORD index = 0; index < table->NumberOfFunctions; ++index) {
        if (functions[index] == 0) continue;
        DWORD ordinal = table->Base + index;
        if (ordinal > 0xffff) return false;
        if (GetProcAddress(proxy, MAKEINTRESOURCEA(static_cast<WORD>(ordinal)))) continue;
        std::fprintf(stderr, "Missing DXGI ordinal: %lu\n", static_cast<unsigned long>(ordinal));
        return false;
    }
    return true;
}

bool create_factory(HMODULE proxy, const char* name, WORD ordinal) {
    using Function = HRESULT(WINAPI*)(REFIID, void**);
    auto named = std::bit_cast<Function>(GetProcAddress(proxy, name));
    auto numbered = std::bit_cast<Function>(GetProcAddress(proxy, MAKEINTRESOURCEA(ordinal)));
    if (!named || !numbered) return false;
    for (Function create : {named, numbered}) {
        void* output = nullptr;
        HRESULT result = create(IID_IDXGIFactory1, &output);
        if (FAILED(result) || !output) {
            std::fprintf(stderr, "%s failed: 0x%08lx\n", name,
                static_cast<unsigned long>(result));
            return false;
        }
        static_cast<IDXGIFactory1*>(output)->Release();
    }
    return true;
}

WORD factory_ordinal(HMODULE system, const char* wanted) {
    auto* table = export_table(system);
    if (!table) return 0;
    auto* base = reinterpret_cast<const BYTE*>(system);
    auto* names = reinterpret_cast<const DWORD*>(base + table->AddressOfNames);
    auto* ordinals = reinterpret_cast<const WORD*>(base + table->AddressOfNameOrdinals);
    for (DWORD index = 0; index < table->NumberOfNames; ++index) {
        auto* name = reinterpret_cast<const char*>(base + names[index]);
        if (std::strcmp(name, wanted) != 0) continue;
        return static_cast<WORD>(table->Base + ordinals[index]);
    }
    return 0;
}
}

int main(int argument_count, char** arguments) {
    if (argument_count != 2) return 1;
    HMODULE proxy = LoadLibraryA(arguments[1]);
    HMODULE system = load_system_dxgi();
    if (!proxy || !system) {
        std::fprintf(stderr, "DXGI LoadLibrary failed: %lu\n", GetLastError());
        return 2;
    }
    if (!check_exports(system, proxy)) return 3;
    WORD ordinal = factory_ordinal(system, "CreateDXGIFactory1");
    if (ordinal == 0 || !create_factory(proxy, "CreateDXGIFactory1", ordinal)) return 4;
    FreeLibrary(system);
    FreeLibrary(proxy);
    return 0;
}
