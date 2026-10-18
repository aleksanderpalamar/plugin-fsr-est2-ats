#include "raytracing/image.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace neuralfx::raytracing {
namespace {
unsigned char encode(double value) noexcept {
    double bounded = std::clamp(value, 0.0, 1.0);
    return static_cast<unsigned char>(std::lround(std::sqrt(bounded) * 255.0));
}
}

bool write_ppm(std::ostream& output, const Image& image) {
    if (image.width <= 0 || image.height <= 0 ||
        image.pixels.size() != static_cast<size_t>(image.width) *
            static_cast<size_t>(image.height)) return false;
    output << "P6\n" << image.width << ' ' << image.height << "\n255\n";
    for (Vec3 color : image.pixels) {
        std::array<char, 3> pixel{static_cast<char>(encode(color.x)),
            static_cast<char>(encode(color.y)), static_cast<char>(encode(color.z))};
        output.write(pixel.data(), static_cast<std::streamsize>(pixel.size()));
    }
    return output.good();
}
}
