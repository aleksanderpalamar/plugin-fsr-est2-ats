#pragma once

#include "raytracing/geometry.hpp"

#include <optional>

namespace neuralfx::raytracing {
class Camera {
public:
    static std::optional<Camera> look_at(Vec3 position, Vec3 target,
        Vec3 up, double vertical_fov_degrees) noexcept;
    Ray ray_for_pixel(int x, int y, int width, int height) const noexcept;

private:
    Camera(Vec3 position, Vec3 forward, Vec3 right, Vec3 up, double field_of_view) noexcept;

    Vec3 position_;
    Vec3 forward_;
    Vec3 right_;
    Vec3 up_;
    double field_of_view_;
};
}
