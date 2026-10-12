#pragma once

#include <windows.h>
#include <atomic>

struct SystemDxgi {
    HMODULE module = nullptr;
    DWORD search_error = ERROR_SUCCESS;
    DWORD fallback_error = ERROR_SUCCESS;
};

class SystemDxgiProvider {
public:
    using Loader = SystemDxgi (*)() noexcept;

    explicit SystemDxgiProvider(Loader loader) noexcept : loader_(loader) {}
    SystemDxgi get() noexcept;

private:
    Loader loader_;
    std::atomic<HMODULE> module_{nullptr};
};
