#include "raytracing/image.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace neuralfx::raytracing {
namespace {
unsigned char encode(double value) noexcept {
    double bounded = std::clamp(value, 0.0, 1.0);
    return static_cast<unsigned char>(std::lround(std::sqrt(bounded) * 255.0));
}
}

std::optional<Image> render(const SceneQuery& scene, const Camera& camera,
    int width, int height, const Settings& settings) {
    if (width <= 0 || height <= 0) return std::nullopt;
    size_t columns = static_cast<size_t>(width);
    size_t rows = static_cast<size_t>(height);
    if (columns > std::numeric_limits<size_t>::max() / rows) return std::nullopt;
    Image image{width, height, std::vector<Vec3>(columns * rows)};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Ray ray = camera.ray_for_pixel(x, y, width, height);
            image.pixels[static_cast<size_t>(y) * columns + x] = trace(scene, ray, settings);
        }
    }
    return image;
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
