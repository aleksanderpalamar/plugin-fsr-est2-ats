#pragma once

#include <filesystem>
#include <string_view>

namespace neuralfx {
std::filesystem::path module_directory();
void log(std::string_view message) noexcept;
}
