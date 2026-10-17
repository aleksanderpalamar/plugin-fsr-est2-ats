#include "raytracing/image.hpp"
#include "raytracing/scene.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace neuralfx::raytracing;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

Triangle surface(double x, double z, double radius, size_t material) {
    return {std::array<Vec3, 3>{{{x - radius, -radius, z},
        {x + radius, -radius, z}, {x, radius, z}}}, std::nullopt, material};
}

void directional_light_cases() {
    DirectionalLight light{{0, 3, 4}, {1, 1, 1}, 1.0};
    require(std::abs(length(light.to_light()) - 1.0) < 1e-12,
        "directional light normalized at construction");
    DirectionalLight changed;
    changed.set_direction({0, 0, 2});
    require(std::abs(length(changed.to_light()) - 1.0) < 1e-12,
        "directional light normalized when changed");
}

void material_cases() {
    bool rejected = false;
    try {
        Scene invalid({surface(0, -3, 1, 1)}, {{{1, 0, 0}, 0.0}}, {});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "invalid material rejected during scene construction");

    Scene scene({surface(0, -3, 1, 0)}, {{{1, 0, 0}, 0.0}}, {});
    rejected = false;
    try {
        scene.rebuild({surface(0, -4, 1, 1)});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "invalid material rejected during rebuild");
    auto triangles = scene.geometry().triangles();
    require(triangles.size() == 1 && triangles[0].positions[0].z == -3.0,
        "rejected rebuild preserves geometry");
}

void image_cases() {
    Image image{2, 1, {{1, 0, 0}, {0.25, 0.25, 0.25}}};
    std::ostringstream stream;
    require(write_ppm(stream, image), "PPM output");
    std::string bytes = stream.str();
    require(bytes.starts_with("P6\n2 1\n255\n"), "PPM header");
    require(bytes.size() == 11 + 6, "PPM pixel payload");
    require(static_cast<unsigned char>(bytes[11]) == 255 && static_cast<unsigned char>(bytes[14]) == 128,
        "PPM encodes with gamma 2");
    Image mismatched{2, 2, {{0, 0, 0}}};
    std::ostringstream ignored;
    require(!write_ppm(ignored, mismatched), "mismatched image rejected");
}
}

int main() {
    try {
        directional_light_cases();
        material_cases();
        image_cases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
