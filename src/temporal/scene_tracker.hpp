#pragma once

#include "temporal/camera_capture.hpp"
#include "temporal/scene_recognizer.hpp"

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
};

class SceneTracker {
public:
    void set_active(bool active) noexcept { active_.store(active, std::memory_order_release); }
    bool active() const noexcept { return active_.load(std::memory_order_acquire); }
    void start_frame(ID3D11Texture2D* backbuffer) noexcept;
    void targets_bound(ID3D11DeviceContext* context) noexcept;
    void draw(ID3D11DeviceContext* context) noexcept;
    SceneSample finish_frame(ID3D11DeviceContext* context) noexcept;

private:
    std::mutex mutex_;
    std::atomic_bool active_{false};
    bool recording_ = false;
    bool captured_ = false;
    std::uint64_t index_ = 0;
    SceneRecognizer recognizer_;
    CameraCapture camera_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_;
};

SceneTracker& scene_tracker() noexcept;
bool install_scene_hooks(ID3D11DeviceContext* context) noexcept;
void scene_targets_bound(ID3D11DeviceContext* context) noexcept;
}
