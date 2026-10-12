#include <windows.h>
#include <cstdio>

int main(int count, char** arguments) {
    if (count != 2) return 1;
    HMODULE module = LoadLibraryA(arguments[1]);
    if (!module) {
        std::fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError());
        return 2;
    }
    return FreeLibrary(module) ? 0 : 3;
}
