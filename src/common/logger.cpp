#include "common/logger.hpp"

#include <windows.h>
#include <algorithm>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {
struct LastErrorGuard {
    DWORD error = GetLastError();
    ~LastErrorGuard() { SetLastError(error); }
};

bool append_path(wchar_t (&path)[MAX_PATH], DWORD& length, const wchar_t* suffix) noexcept {
    for (DWORD index = 0; suffix[index] != L'\0'; ++index) {
        if (length + 1 >= MAX_PATH) return false;
        path[length++] = suffix[index];
    }
    path[length] = L'\0';
    return true;
}

bool ensure_directory(const wchar_t* path) noexcept {
    if (CreateDirectoryW(path, nullptr)) return true;
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

bool log_path(wchar_t (&path)[MAX_PATH]) noexcept {
    DWORD length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return false;
    while (length > 0 && path[length - 1] != L'\\' && path[length - 1] != L'/') --length;
    if (length == 0) return false;
    path[length] = L'\0';
    if (!append_path(path, length, L"NeuralFX") || !ensure_directory(path)) return false;
    if (!append_path(path, length, L"\\logs") || !ensure_directory(path)) return false;
    return append_path(path, length, L"\\neuralfx.log");
}

DWORD format_line(char (&line)[2048], std::string_view message) noexcept {
    constexpr std::string_view prefix = "[NeuralFX] ";
    DWORD length = 0;
    for (char value : prefix) line[length++] = value;
    const auto count = std::min(message.size(), sizeof(line) - length - 1);
    for (std::size_t index = 0; index < count; ++index) line[length++] = message[index];
    line[length++] = '\n';
    return length;
}
}

namespace neuralfx {
std::filesystem::path module_directory() {
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    return std::filesystem::path(buffer).parent_path();
}

void log(std::string_view message) noexcept {
    LastErrorGuard last_error;
    wchar_t path[MAX_PATH]{};
    if (!log_path(path)) return;
    HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    char line[2048]{};
    DWORD length = format_line(line, message);
    DWORD written = 0;
    if (!WriteFile(file, line, length, &written, nullptr) || written != length) {
        CloseHandle(file);
        return;
    }
    CloseHandle(file);
}
}
