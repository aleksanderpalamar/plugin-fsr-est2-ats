#pragma once

#include "upscale/capture_images.hpp"

#include <filesystem>

namespace neuralfx::upscale {
bool write_bmp(const std::filesystem::path& path, const RgbImage& image) noexcept;
}
