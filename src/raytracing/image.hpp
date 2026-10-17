#pragma once

#include "raytracing/vec3.hpp"

#include <ostream>
#include <vector>

namespace neuralfx::raytracing {
struct Image {
    int width = 0;
    int height = 0;
    std::vector<Vec3> pixels;

    Vec3 at(int x, int y) const noexcept {
        return pixels[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
    }
};

bool write_ppm(std::ostream& output, const Image& image);
}
