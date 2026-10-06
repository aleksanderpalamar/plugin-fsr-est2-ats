#pragma once

#include "raytracing/tracer.hpp"

#include <optional>
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

std::optional<Image> render(const SceneQuery& scene, const Camera& camera,
    int width, int height, const Settings& settings);
bool write_ppm(std::ostream& output, const Image& image);
}
