#pragma once

#include <windows.h>
#include <cstdio>
#include <string>

namespace spike {
inline bool report(const char* step, HRESULT result, const std::string& detail = {}) {
    std::printf("%s result=%s hr=0x%08lx %s\n", step, SUCCEEDED(result) ? "ok" : "fail",
        static_cast<unsigned long>(result), detail.c_str());
    std::fflush(stdout);
    return SUCCEEDED(result);
}

inline std::string narrow(const wchar_t* text) {
    std::string output;
    for (; text && *text; ++text) output.push_back(*text < 128 ? static_cast<char>(*text) : '?');
    return output;
}
}
