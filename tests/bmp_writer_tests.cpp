#include "upscale/bmp_writer.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace neuralfx::upscale;

int main() {
    RgbImage image{3, 2, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}};
    auto path = std::filesystem::temp_directory_path() / "neuralfx_bmp_writer_test.bmp";
    if (!write_bmp(path, image)) return 1;
    std::ifstream file(path, std::ios::binary);
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    std::filesystem::remove(path);
    constexpr std::size_t row = 12;
    if (data.size() != 54 + row * 2) return 2;
    if (data[0] != 'B' || data[1] != 'M') return 3;
    std::uint32_t width = 0;
    std::int32_t height = 0;
    std::memcpy(&width, data.data() + 18, 4);
    std::memcpy(&height, data.data() + 22, 4);
    if (width != 3 || height != 2) return 4;
    if (data[54] != 12 || data[55] != 11 || data[56] != 10) return 5;
    if (data[54 + row] != 3 || data[54 + row + 1] != 2 || data[54 + row + 2] != 1) return 6;
    RgbImage empty;
    return write_bmp(path, empty) ? 7 : 0;
}
