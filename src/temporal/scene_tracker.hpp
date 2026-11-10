#pragma once

#include "temporal/camera_capture.hpp"
#include "temporal/pass_listener.hpp"
#include "temporal/resolve_capture.hpp"
#include "temporal/scene_recognizer.hpp"
#include "temporal/stage_times.hpp"

#include <d3d11.h>
#include <atomic>
#include <mutex>
#include <wrl/client.h>

namespace neuralfx::temporal {
struct SceneSample {
    std::uint64_t index = 0;
    SceneFrame frame;
    Microsoft::WRL::ComPtr<ID3D11Buffer> camera;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth;
    std::vector<CameraObservation> observations;
    Microsoft::WRL::ComPtr<ID3D11Buffer> resolve;
    std::vector<FrameFloats> resolves;
    StageTimes times;
};

class SceneTracker {
public:
    void set_active(bool active) noexcept { active_.store(active, std::memory_order_release); }
    bool active() const noexcept { return active_.load(std::memory_order_acquire); }
    void start_frame(ID3D11Texture2D* backbuffer) noexcept;
    void targets_bound(ID3D11DeviceContext* context) noexcept;
    void draw(ID3D11DeviceContext* context) noexcept;
    SceneSample finish_frame(ID3D11DeviceContext* context) noexcept;
    void set_listener(PassListener* listener) noexcept;

private:
    std::mutex mutex_;
    std::atomic_bool active_{false};
    bool recording_ = false;
    bool captured_ = false;
    bool resolved_ = false;
    std::int64_t capture_us_ = 0;
    std::uint64_t index_ = 0;
    SceneRecognizer recognizer_;
    CameraCapture camera_;
    ResolveCapture resolve_;
    PassListener* listener_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_;
};

SceneTracker& scene_tracker() noexcept;
bool install_scene_hooks(ID3D11DeviceContext* context) noexcept;
void scene_targets_bound(ID3D11DeviceContext* context) noexcept;
}
