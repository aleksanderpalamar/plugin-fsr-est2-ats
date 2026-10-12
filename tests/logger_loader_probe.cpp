#include "common/logger.hpp"

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) neuralfx::log("DXGI loader regression");
    return TRUE;
}
