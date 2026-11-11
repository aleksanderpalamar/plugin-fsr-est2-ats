#include "upscale/frame_contract.hpp"

#include <cmath>

namespace neuralfx::upscale {
namespace {
constexpr float max_jitter = 1.0f;

bool sizes_valid(const FrameContract& contract) {
    bool present = contract.render.width > 0 && contract.render.height > 0 && contract.output.width > 0 &&
        contract.output.height > 0;
    return present && contract.render.width <= contract.output.width && contract.render.height <= contract.output.height;
}

bool camera_valid(const CameraParameters& camera) {
    bool finite = std::isfinite(camera.near_plane) && std::isfinite(camera.far_plane) && std::isfinite(camera.vertical_fov);
    return finite && camera.near_plane > 0.0f && camera.far_plane > camera.near_plane && camera.vertical_fov > 0.0f &&
        camera.vertical_fov < 3.1415926f;
}

bool numbers_valid(const FrameContract& contract) {
    bool jitter = std::fabs(contract.jitter.x) <= max_jitter && std::fabs(contract.jitter.y) <= max_jitter;
    return jitter && std::isfinite(contract.frame_time_ms) && contract.frame_time_ms > 0.0f;
}
}

ContractFault diagnose(const FrameContract& contract) noexcept {
    if (!sizes_valid(contract)) return ContractFault::Sizes;
    if (!numbers_valid(contract)) return ContractFault::Numbers;
    if (!camera_valid(contract.camera)) return ContractFault::Camera;
    if (contract.inputs != InputAvailability::Complete || contract.motion != MotionQuality::Valid)
        return ContractFault::Inputs;
    return ContractFault::None;
}

UpscaleStatus validate(const FrameContract& contract) noexcept {
    return diagnose(contract) == ContractFault::None ? UpscaleStatus::Ready : UpscaleStatus::InvalidInput;
}

const char* fault_name(ContractFault fault) noexcept {
    switch (fault) {
    case ContractFault::None: return "none";
    case ContractFault::Sizes: return "sizes";
    case ContractFault::Numbers: return "numbers";
    case ContractFault::Camera: return "camera";
    case ContractFault::Inputs: return "inputs";
    }
    return "unknown";
}

const char* status_name(UpscaleStatus status) noexcept {
    switch (status) {
    case UpscaleStatus::Ready: return "ready";
    case UpscaleStatus::Unsupported: return "unsupported";
    case UpscaleStatus::InvalidInput: return "invalid-input";
    case UpscaleStatus::UnavailableDependency: return "unavailable-dependency";
    case UpscaleStatus::DeviceLost: return "device-lost";
    case UpscaleStatus::BackendFailure: return "backend-failure";
    }
    return "unknown";
}
}
