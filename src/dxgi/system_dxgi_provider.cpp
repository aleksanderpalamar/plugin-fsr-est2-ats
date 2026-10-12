#include "dxgi/system_dxgi_provider.hpp"

SystemDxgi SystemDxgiProvider::get() noexcept {
    HMODULE module = module_.load(std::memory_order_acquire);
    if (module) return {module, ERROR_SUCCESS, ERROR_SUCCESS};
    if (!loader_) return {nullptr, ERROR_INVALID_FUNCTION, ERROR_SUCCESS};
    SystemDxgi loaded = loader_();
    if (!loaded.module) return loaded;
    HMODULE expected = nullptr;
    if (module_.compare_exchange_strong(expected, loaded.module,
            std::memory_order_acq_rel, std::memory_order_acquire)) return loaded;
    FreeLibrary(loaded.module);
    return {expected, ERROR_SUCCESS, ERROR_SUCCESS};
}
