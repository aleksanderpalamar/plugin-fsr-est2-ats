#include "upscale/frame_contract.hpp"

#include <cmath>
#include <iostream>

using namespace neuralfx::upscale;

namespace {
FrameContract valid_contract() {
    FrameContract contract;
    contract.render = {1440, 1080};
    contract.output = {1920, 1080};
    contract.jitter = {0.25f, -0.4f};
    contract.frame_time_ms = 16.6f;
    contract.inputs = InputAvailability::Complete;
    contract.motion = MotionQuality::Valid;
    contract.camera = {0.1f, 1600.0f, 0.9f};
    return contract;
}

int ready_case() {
    return validate(valid_contract()) == UpscaleStatus::Ready ? 0 : 10;
}

int sizes_case() {
    auto contract = valid_contract();
    contract.render = {0, 1080};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 20;
    contract = valid_contract();
    contract.output = {1920, 0};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 21;
    contract = valid_contract();
    contract.render = {2000, 1080};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 22;
    contract = valid_contract();
    contract.render = {1920, 1080};
    return validate(contract) == UpscaleStatus::Ready ? 0 : 23;
}

int inputs_case() {
    for (auto missing : {InputAvailability::NoColor, InputAvailability::NoDepth, InputAvailability::NoMotion}) {
        auto contract = valid_contract();
        contract.inputs = missing;
        if (validate(contract) != UpscaleStatus::InvalidInput) return 30;
    }
    auto contract = valid_contract();
    contract.motion = MotionQuality::Invalid;
    return validate(contract) == UpscaleStatus::InvalidInput ? 0 : 31;
}

int numbers_case() {
    auto contract = valid_contract();
    contract.jitter = {std::nanf(""), 0.0f};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 40;
    contract = valid_contract();
    contract.jitter = {0.0f, 1.5f};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 41;
    contract = valid_contract();
    contract.frame_time_ms = 0.0f;
    if (validate(contract) != UpscaleStatus::InvalidInput) return 42;
    contract.frame_time_ms = std::nanf("");
    return validate(contract) == UpscaleStatus::InvalidInput ? 0 : 43;
}

int camera_case() {
    auto contract = valid_contract();
    contract.camera = {0.0f, 1600.0f, 0.9f};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 70;
    contract.camera = {1.0f, 1.0f, 0.9f};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 71;
    contract.camera = {0.1f, 1600.0f, 0.0f};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 72;
    contract.camera = {0.1f, 1600.0f, 3.2f};
    if (validate(contract) != UpscaleStatus::InvalidInput) return 73;
    contract.camera = {0.1f, std::nanf(""), 0.9f};
    return validate(contract) == UpscaleStatus::InvalidInput ? 0 : 74;
}

int diagnose_case() {
    if (diagnose(valid_contract()) != ContractFault::None) return 80;
    auto contract = valid_contract();
    contract.render = {2000, 1080};
    if (diagnose(contract) != ContractFault::Sizes) return 81;
    contract = valid_contract();
    contract.frame_time_ms = 0.0f;
    if (diagnose(contract) != ContractFault::Numbers) return 82;
    contract = valid_contract();
    contract.jitter = {2.0f, 0.0f};
    if (diagnose(contract) != ContractFault::Numbers) return 83;
    contract = valid_contract();
    contract.camera.vertical_fov = 0.0f;
    if (diagnose(contract) != ContractFault::Camera) return 84;
    contract = valid_contract();
    contract.inputs = InputAvailability::NoDepth;
    if (diagnose(contract) != ContractFault::Inputs) return 85;
    contract = valid_contract();
    contract.motion = MotionQuality::Invalid;
    if (diagnose(contract) != ContractFault::Inputs) return 86;
    if (std::string(fault_name(ContractFault::None)) != "none" || std::string(fault_name(ContractFault::Sizes)) != "sizes" ||
        std::string(fault_name(ContractFault::Numbers)) != "numbers" || std::string(fault_name(ContractFault::Camera)) != "camera" ||
        std::string(fault_name(ContractFault::Inputs)) != "inputs") return 87;
    return 0;
}

int reset_case() {
    auto contract = valid_contract();
    contract.reset = true;
    return validate(contract) == UpscaleStatus::Ready ? 0 : 50;
}

int names_case() {
    if (std::string(status_name(UpscaleStatus::Ready)) != "ready") return 60;
    if (std::string(status_name(UpscaleStatus::Unsupported)) != "unsupported") return 61;
    if (std::string(status_name(UpscaleStatus::InvalidInput)) != "invalid-input") return 62;
    if (std::string(status_name(UpscaleStatus::UnavailableDependency)) != "unavailable-dependency") return 63;
    if (std::string(status_name(UpscaleStatus::DeviceLost)) != "device-lost") return 64;
    return std::string(status_name(UpscaleStatus::BackendFailure)) == "backend-failure" ? 0 : 65;
}
}

int main() {
    for (auto test_case : {ready_case, sizes_case, inputs_case, numbers_case, camera_case, diagnose_case, reset_case, names_case}) {
        if (int code = test_case(); code != 0) {
            std::cerr << "frame_contract case failed: " << code << '\n';
            return code;
        }
    }
    return 0;
}
