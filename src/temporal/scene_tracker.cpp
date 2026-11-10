#include "temporal/scene_tracker.hpp"

#include <array>
#include <chrono>
#include <vector>

namespace neuralfx::temporal {
namespace {
using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;

std::int64_t microseconds_since(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
}

struct BoundTargets {
    std::vector<Surface> colors;
    std::optional<Surface> depth;
    ComPtr<ID3D11Texture2D> depth_texture;
};

ComPtr<ID3D11Texture2D> texture_of(ID3D11View* view) {
    ComPtr<ID3D11Resource> resource;
    if (view) view->GetResource(&resource);
    ComPtr<ID3D11Texture2D> texture;
    if (resource) resource.As(&texture);
    return texture;
}

std::optional<Surface> surface_of(ID3D11Texture2D* texture) {
    if (!texture) return std::nullopt;
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    return Surface{reinterpret_cast<std::uintptr_t>(texture), description.Width, description.Height,
        static_cast<std::uint32_t>(description.Format)};
}

BoundTargets bound_targets(ID3D11DeviceContext* context) {
    std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> raw{};
    ComPtr<ID3D11DepthStencilView> depth;
    context->OMGetRenderTargets(static_cast<UINT>(raw.size()), raw.data(), &depth);
    BoundTargets bound;
    for (auto* view : raw) {
        ComPtr<ID3D11RenderTargetView> owned;
        owned.Attach(view);
        if (auto surface = surface_of(texture_of(view).Get())) bound.colors.push_back(*surface);
    }
    bound.depth_texture = texture_of(depth.Get());
    bound.depth = surface_of(bound.depth_texture.Get());
    return bound;
}
}

void SceneTracker::start_frame(ID3D11Texture2D* backbuffer) noexcept {
    std::lock_guard lock(mutex_);
    auto surface = surface_of(backbuffer);
    recording_ = active() && surface.has_value();
    if (!recording_) return;
    recognizer_.begin_frame(*surface);
    ++index_;
    captured_ = false;
    resolved_ = false;
    capture_us_ = 0;
    depth_.Reset();
}

void SceneTracker::targets_bound(ID3D11DeviceContext* context) noexcept {
    if (!active()) return;
    std::lock_guard lock(mutex_);
    if (!recording_) return;
    try {
        auto bound = bound_targets(context);
        if (recognizer_.bind(bound.colors, bound.depth) == PassRole::SceneGeometry) depth_ = bound.depth_texture;
    } catch (...) {
        recording_ = false;
    }
}

void SceneTracker::draw(ID3D11DeviceContext* context) noexcept {
    if (!active()) return;
    std::lock_guard lock(mutex_);
    if (!recording_) return;
    auto action = recognizer_.draw();
    auto start = Clock::now();
    if (action == DrawAction::ReadCamera) captured_ = camera_.capture(context, index_);
    if (action == DrawAction::ReadResolve) resolved_ = resolve_.capture(context, index_);
    if (action != DrawAction::None) capture_us_ += microseconds_since(start);
    if (action == DrawAction::None || !listener_) return;
    listener_->first_draw(context, index_,
        action == DrawAction::ReadCamera ? PassRole::SceneLighting : PassRole::TemporalResolve);
}

void SceneTracker::set_listener(PassListener* listener) noexcept {
    std::lock_guard lock(mutex_);
    listener_ = listener;
}

SceneSample SceneTracker::finish_frame(ID3D11DeviceContext* context) noexcept {
    std::lock_guard lock(mutex_);
    if (!recording_) return {};
    recording_ = false;
    auto start = Clock::now();
    SceneSample sample{index_, recognizer_.frame(), nullptr, depth_, camera_.collect(context)};
    sample.times.camera_collect_us = microseconds_since(start);
    start = Clock::now();
    sample.resolves = resolve_.collect(context);
    sample.times.resolve_collect_us = microseconds_since(start);
    sample.times.capture_us = capture_us_;
    if (captured_) sample.camera = camera_.camera();
    if (resolved_) sample.resolve = resolve_.resolve();
    return sample;
}

SceneTracker& scene_tracker() noexcept {
    static SceneTracker tracker;
    return tracker;
}
}
