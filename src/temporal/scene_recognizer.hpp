#pragma once

#include "temporal/projection_jitter.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace neuralfx::temporal {
struct Surface {
    std::uint64_t id = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t format = 0;
};

enum class PassRole { Other, SceneGeometry, SceneLighting, TemporalResolve, Output };
enum class DrawAction { None, ReadCamera, ReadResolve, ReadOutput };

struct SceneFrame {
    std::optional<RenderSize> render;
    std::optional<std::uint64_t> depth;
    bool camera_requested = false;
    bool temporal_resolve = false;
    bool resolve_drawn = false;
    bool output_drawn = false;
};

class SceneRecognizer {
public:
    void begin_frame(Surface backbuffer) noexcept;
    PassRole bind(std::span<const Surface> colors, std::optional<Surface> depth) noexcept;
    DrawAction draw() noexcept;
    SceneFrame frame() const noexcept { return frame_; }

private:
    PassRole classify(std::span<const Surface> colors, const std::optional<Surface>& depth) const noexcept;
    void remember_geometry(const Surface& depth) noexcept;

    Surface backbuffer_;
    SceneFrame frame_;
    PassRole current_ = PassRole::Other;
};
}
