#include "temporal/scene_recognizer.hpp"

#include <algorithm>

namespace neuralfx::temporal {
namespace {
constexpr std::size_t geometry_targets = 4;
constexpr std::size_t paired_targets = 2;

bool same_size(const Surface& left, const Surface& right) {
    return left.width == right.width && left.height == right.height;
}

bool all_match(std::span<const Surface> colors, const Surface& reference) {
    return std::all_of(colors.begin(), colors.end(),
        [&](const Surface& color) { return same_size(color, reference); });
}

bool is_render_size(const Surface& surface, RenderSize render) {
    return surface.width == render.width && surface.height == render.height;
}
}

void SceneRecognizer::begin_frame(Surface backbuffer) noexcept {
    backbuffer_ = backbuffer;
    frame_ = {};
    current_ = PassRole::Other;
}

PassRole SceneRecognizer::bind(std::span<const Surface> colors, std::optional<Surface> depth) noexcept {
    current_ = classify(colors, depth);
    if (current_ == PassRole::SceneGeometry) remember_geometry(*depth);
    if (current_ == PassRole::TemporalResolve) frame_.temporal_resolve = true;
    return current_;
}

DrawAction SceneRecognizer::draw() noexcept {
    if (current_ != PassRole::SceneLighting || frame_.camera_requested) return DrawAction::None;
    frame_.camera_requested = true;
    return DrawAction::ReadCamera;
}

PassRole SceneRecognizer::classify(std::span<const Surface> colors, const std::optional<Surface>& depth) const noexcept {
    if (colors.size() == 1 && !depth && colors.front().id == backbuffer_.id) return PassRole::Output;
    if (colors.size() >= geometry_targets && depth && all_match(colors, *depth)) return PassRole::SceneGeometry;
    if (!frame_.depth || colors.size() != paired_targets) return PassRole::Other;
    if (depth && depth->id == *frame_.depth) return PassRole::SceneLighting;
    bool resolve = !depth && frame_.camera_requested &&
        std::all_of(colors.begin(), colors.end(), [&](const Surface& color) { return is_render_size(color, *frame_.render); });
    return resolve ? PassRole::TemporalResolve : PassRole::Other;
}

void SceneRecognizer::remember_geometry(const Surface& depth) noexcept {
    frame_.render = RenderSize{depth.width, depth.height};
    frame_.depth = depth.id;
}
}
