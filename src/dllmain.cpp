#include <windows.h>
#include "screenshots/steam_interception.hpp"

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        neuralfx::install_steam_callback_interception();
    }
    return TRUE;
}
