#include "raytracing/camera.hpp"

#include <cmath>
#include <numbers>

namespace neuralfx::raytracing {
Camera::Camera(Vec3 position, Vec3 forward, Vec3 right, Vec3 up, double field_of_view) noexcept
    : position_(position), forward_(forward), right_(right), up_(up),
      field_of_view_(field_of_view) {}

std::optional<Camera> Camera::look_at(Vec3 position, Vec3 target,
    Vec3 up, double vertical_fov_degrees) noexcept {
    if (!std::isfinite(vertical_fov_degrees) || vertical_fov_degrees <= 0.0 ||
        vertical_fov_degrees >= 180.0) return std::nullopt;
    Vec3 forward = normalized(target - position);
    if (length(forward) == 0.0) return std::nullopt;
    Vec3 right = normalized(cross(forward, up));
    if (length(right) == 0.0) return std::nullopt;
    return Camera(position, forward, right, cross(right, forward), vertical_fov_degrees);
}

Ray Camera::ray_for_pixel(int x, int y, int width, int height) const noexcept {
    double aspect = static_cast<double>(width) / height;
    double scale = std::tan(field_of_view_ * std::numbers::pi / 360.0);
    double horizontal = (2.0 * (x + 0.5) / width - 1.0) * aspect * scale;
    double vertical = (1.0 - 2.0 * (y + 0.5) / height) * scale;
    return {position_, normalized(forward_ + right_ * horizontal + up_ * vertical)};
}
}
