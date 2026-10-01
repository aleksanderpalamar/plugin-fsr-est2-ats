#include "common/logger.hpp"

#include <windows.h>
#include <fstream>
#include <mutex>
#include <string>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace neuralfx {
std::filesystem::path module_directory() {
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    return std::filesystem::path(buffer).parent_path();
}

void log(std::string_view message) noexcept {
    static std::mutex mutex;
    try {
        std::lock_guard lock(mutex);
        auto directory = module_directory() / L"NeuralFX" / L"logs";
        std::filesystem::create_directories(directory);
        std::ofstream file(directory / L"neuralfx.log", std::ios::app);
        if (file) file << "[NeuralFX] " << message << '\n';
    } catch (...) {
    }
}
}
