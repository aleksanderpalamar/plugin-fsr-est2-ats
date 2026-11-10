#include "upscale/upscale_session.hpp"

#include <iostream>
#include <vector>

using namespace neuralfx::upscale;
namespace temporal = neuralfx::temporal;

namespace {
struct FakeBackend final : UpscalingBackend {
    std::vector<ProviderVersion> available;
    std::string reported;
    UpscaleStatus open_result = UpscaleStatus::Ready;
    UpscaleStatus upscale_result = UpscaleStatus::Ready;
    int provider_queries = 0;
    int opens = 0;
    int closes = 0;
    int upscales = 0;
    bool last_reset = false;
    std::string opened;
    OpenParameters last_parameters;

    std::vector<ProviderVersion> providers() override {
        ++provider_queries;
        return available;
    }
    UpscaleStatus open(const ProviderVersion& provider, const OpenParameters& parameters) override {
        last_parameters = parameters;
        ++opens;
        opened = provider.name;
        if (reported.empty()) reported = provider.name;
        return open_result;
    }
    std::string provider_name() const override { return reported; }
    UpscaleStatus upscale(const FrameContract& contract, const FrameResources&) override {
        ++upscales;
        last_reset = contract.reset;
        return upscale_result;
    }
    void close() override { ++closes; }
};

const temporal::RenderSize render{1440, 1080};
const temporal::RenderSize output{1920, 1080};
const OpenParameters parameters{render, output, true, true};
const FrameResources resources{reinterpret_cast<ID3D11Texture2D*>(1), reinterpret_cast<ID3D11Texture2D*>(2),
    reinterpret_cast<ID3D11Texture2D*>(3), reinterpret_cast<ID3D11Texture2D*>(4)};

FrameContract good_frame() {
    FrameContract contract;
    contract.render = render;
    contract.output = output;
    contract.frame_time_ms = 16.6f;
    contract.camera = {0.1f, 1600.0f, 0.9f};
    return contract;
}

FakeBackend proton() {
    FakeBackend backend;
    backend.available = {{0xA, "3.1.6"}, {0xB, "2.3.4"}};
    return backend;
}

int off_case() {
    auto backend = proton();
    UpscaleSession session(backend, UpscaleMode::Off);
    if (session.open(parameters) != UpscaleStatus::Unsupported) return 10;
    if (session.report().state != SessionState::Off || backend.provider_queries != 0 || backend.opens != 0) return 11;
    return session.process(good_frame(), resources) == UpscaleStatus::Unsupported && backend.upscales == 0 ? 0 : 12;
}

int strict_unavailable_case() {
    auto backend = proton();
    UpscaleSession session(backend, UpscaleMode::Strict);
    if (session.open(parameters) != UpscaleStatus::Unsupported) return 20;
    const auto& report = session.report();
    if (report.state != SessionState::Inactive || report.reason != SelectionReason::PreferredUnavailable) return 21;
    if (!report.active_version.empty() || backend.opens != 0) return 22;
    return session.process(good_frame(), resources) == UpscaleStatus::Unsupported && backend.upscales == 0 ? 0 : 23;
}

int auto_fallback_case() {
    auto backend = proton();
    UpscaleSession session(backend, UpscaleMode::Auto);
    if (session.open(parameters) != UpscaleStatus::Ready) return 30;
    const auto& report = session.report();
    if (report.state != SessionState::Active || report.active_version != "3.1.6") return 31;
    if (report.reason != SelectionReason::Fallback || backend.opened != "3.1.6") return 32;
    return session.process(good_frame(), resources) == UpscaleStatus::Ready && backend.upscales == 1 ? 0 : 33;
}

int native_case() {
    FakeBackend backend;
    backend.available = {{0xC, "4.1.2"}, {0xA, "3.1.6"}};
    UpscaleSession strict(backend, UpscaleMode::Strict);
    if (strict.open(parameters) != UpscaleStatus::Ready || strict.report().active_version != "4.1.2") return 40;
    UpscaleSession automatic(backend, UpscaleMode::Auto);
    return automatic.open(parameters) == UpscaleStatus::Ready && backend.opened == "4.1.2" ? 0 : 41;
}

int version_mismatch_case() {
    auto backend = proton();
    FakeBackend native;
    native.available = {{0xC, "4.1.2"}};
    native.reported = "3.1.6";
    UpscaleSession session(native, UpscaleMode::Strict);
    if (session.open(parameters) != UpscaleStatus::BackendFailure) return 50;
    if (session.report().state != SessionState::Failed || !session.report().active_version.empty()) return 51;
    return native.closes == 1 ? 0 : 52;
}

int open_failure_case() {
    auto backend = proton();
    backend.open_result = UpscaleStatus::UnavailableDependency;
    UpscaleSession session(backend, UpscaleMode::Auto);
    if (session.open(parameters) != UpscaleStatus::UnavailableDependency) return 60;
    return session.report().state == SessionState::Failed && backend.closes == 1 ? 0 : 61;
}

int invalid_frame_case() {
    auto backend = proton();
    UpscaleSession session(backend, UpscaleMode::Auto);
    session.open(parameters);
    auto broken = good_frame();
    broken.motion = MotionQuality::Invalid;
    if (session.process(broken, resources) != UpscaleStatus::InvalidInput || backend.upscales != 0) return 70;
    if (session.process(good_frame(), resources) != UpscaleStatus::Ready || !backend.last_reset) return 71;
    if (session.process(good_frame(), resources) != UpscaleStatus::Ready || backend.last_reset) return 72;
    auto incomplete = FrameResources{};
    return session.process(good_frame(), incomplete) == UpscaleStatus::InvalidInput ? 0 : 73;
}

int backend_failure_case() {
    auto backend = proton();
    UpscaleSession session(backend, UpscaleMode::Auto);
    session.open(parameters);
    backend.upscale_result = UpscaleStatus::DeviceLost;
    if (session.process(good_frame(), resources) != UpscaleStatus::DeviceLost) return 80;
    if (session.report().state != SessionState::Failed || backend.closes != 1) return 81;
    return session.process(good_frame(), resources) == UpscaleStatus::Unsupported && backend.upscales == 1 ? 0 : 82;
}

int parameters_case() {
    auto backend = proton();
    UpscaleSession session(backend, UpscaleMode::Auto);
    session.open(parameters);
    const auto& seen = backend.last_parameters;
    bool same = seen.max_render.width == 1440 && seen.max_output.width == 1920 && seen.depth_inverted && seen.color_non_linear;
    return same ? 0 : 95;
}

int close_case() {
    auto backend = proton();
    UpscaleSession session(backend, UpscaleMode::Auto);
    session.open(parameters);
    session.close();
    session.close();
    return backend.closes == 1 && session.report().state == SessionState::Inactive ? 0 : 90;
}
}

int main() {
    for (auto test_case : {off_case, strict_unavailable_case, auto_fallback_case, native_case, version_mismatch_case,
             open_failure_case, invalid_frame_case, backend_failure_case, parameters_case, close_case}) {
        if (int code = test_case(); code != 0) {
            std::cerr << "upscale_session case failed: " << code << '\n';
            return code;
        }
    }
    return 0;
}
