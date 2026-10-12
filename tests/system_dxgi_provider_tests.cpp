#include "dxgi/system_dxgi_provider.hpp"

namespace {
int attempts = 0;

SystemDxgi fail_then_succeed() noexcept {
    ++attempts;
    if (attempts == 1) return {nullptr, ERROR_NOT_FOUND, ERROR_NOT_FOUND};
    return {GetModuleHandleW(nullptr), ERROR_SUCCESS, ERROR_SUCCESS};
}
}

int main() {
    SystemDxgiProvider provider(fail_then_succeed);
    if (provider.get().module) return 1;
    if (!provider.get().module) return 2;
    if (!provider.get().module || attempts != 2) return 3;
    return 0;
}
