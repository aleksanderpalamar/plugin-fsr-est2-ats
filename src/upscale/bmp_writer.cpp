#include "upscale/bmp_writer.hpp"

#include <array>
#include <fstream>

namespace neuralfx::upscale {
namespace {
constexpr std::uint32_t header_bytes = 54;

void put32(std::array<char, header_bytes>& header, std::size_t at, std::uint32_t value) {
    for (std::size_t byte = 0; byte < 4; ++byte) header[at + byte] = static_cast<char>((value >> (8 * byte)) & 0xFF);
}

std::array<char, header_bytes> make_header(std::uint32_t width, std::uint32_t height, std::uint32_t data_bytes) {
    std::array<char, header_bytes> header{};
    header[0] = 'B';
    header[1] = 'M';
    put32(header, 2, header_bytes + data_bytes);
    put32(header, 10, header_bytes);
    put32(header, 14, 40);
    put32(header, 18, width);
    put32(header, 22, height);
    header[26] = 1;
    header[28] = 24;
    put32(header, 34, data_bytes);
    return header;
}
}

bool write_bmp(const std::filesystem::path& path, const RgbImage& image) noexcept {
    std::size_t row = static_cast<std::size_t>(image.width) * 3;
    if (image.width == 0 || image.height == 0 || image.bytes.size() != row * image.height) return false;
    try {
        std::size_t padded = (row + 3) & ~static_cast<std::size_t>(3);
        auto header = make_header(image.width, image.height, static_cast<std::uint32_t>(padded * image.height));
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) return false;
        file.write(header.data(), header.size());
        std::vector<char> line(padded, 0);
        for (std::uint32_t y = image.height; y-- > 0;) {
            for (std::uint32_t x = 0; x < image.width; ++x)
                for (std::size_t channel = 0; channel < 3; ++channel)
                    line[x * 3 + channel] = static_cast<char>(image.bytes[y * row + x * 3 + 2 - channel]);
            file.write(line.data(), static_cast<std::streamsize>(line.size()));
        }
        return static_cast<bool>(file);
    } catch (...) {
        return false;
    }
}
}
