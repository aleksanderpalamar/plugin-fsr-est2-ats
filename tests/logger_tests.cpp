#include "common/logger.hpp"

#include <windows.h>

int main() {
    constexpr DWORD expected = ERROR_ACCESS_DENIED;
    SetLastError(expected);
    neuralfx::log("DXGI proxy logger regression");
    return GetLastError() == expected ? 0 : 1;
}
