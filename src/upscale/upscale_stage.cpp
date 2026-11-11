#include "upscale/upscale_stage.hpp"
#include "common/logger.hpp"
#include "renderer/context_state.hpp"
#include "temporal/resolve_review.hpp"

namespace neuralfx::upscale {
namespace {
using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;

constexpr std::uint64_t logged_notes = 48;
constexpr float default_frame_ms = 16.6f;
constexpr float near_plane = 0.2f;
constexpr float far_plane = 1600.0f;

struct OutputBindings {
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<ID3D11Texture2D> backbuffer;
    ComPtr<ID3D11Texture2D> color;
};

std::optional<OutputBindings> bindings(ID3D11DeviceContext* context) {
    OutputBindings found;
    ID3D11RenderTargetView* raw = nullptr;
    context->OMGetRenderTargets(1, &raw, nullptr);
    found.target.Attach(raw);
    ID3D11ShaderResourceView* view = nullptr;
    context->PSGetShaderResources(0, 1, &view);
    ComPtr<ID3D11ShaderResourceView> source;
    source.Attach(view);
    ComPtr<ID3D11Resource> resource;
    if (found.target) found.target->GetResource(&resource);
    if (resource) resource.As(&found.backbuffer);
    resource.Reset();
    if (source) source->GetResource(&resource);
    if (resource) resource.As(&found.color);
    if (!found.target || !found.backbuffer || !found.color) return std::nullopt;
    return found;
}

temporal::RenderSize size_of(ID3D11Texture2D* texture) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    return {description.Width, description.Height};
}

bool same(temporal::RenderSize left, temporal::RenderSize right) {
    return left.width == right.width && left.height == right.height;
}
}

HRESULT UpscaleStage::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    HRESULT result = inputs_.initialize(device, shader_directory);
    return SUCCEEDED(result) ? blit_.initialize(device, shader_directory) : result;
}

void UpscaleStage::note(const std::string& message) noexcept {
    if (notes_ >= logged_notes) return;
    ++notes_;
    try {
        log("Upscale: " + message);
    } catch (...) {
    }
}

void UpscaleStage::begin_frame() noexcept {
    replaced_ = false;
}

void UpscaleStage::observe(const temporal::SceneSample& sample) noexcept {
    if (!sample.frame.render) return;
    try {
        auto report = temporal::review_resolves(sample.resolves, *sample.frame.render);
        for (const auto& item : report.samples) {
            tracker_.observe({item.frame, item.values.jitter});
            if (item.values.vertical_fov > 0.0f) vertical_fov_ = item.values.vertical_fov;
            temporal::JumpLimits limits;
            if (item.values.move_m > limits.distance_m || item.values.turn_deg > limits.angle_deg) pending_reset_ = true;
        }
    } catch (...) {
    }
}

temporal::DrawDecision UpscaleStage::first_draw(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept {
    if (event.role == temporal::PassRole::TemporalResolve) return on_resolve(context, event);
    if (event.role == temporal::PassRole::Output) return on_output(context, event);
    return temporal::DrawDecision::Proceed;
}

temporal::DrawDecision UpscaleStage::on_resolve(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept {
    if (!event.render) return temporal::DrawDecision::Proceed;
    bool wanted = ready_ && vertical_fov_ > 0.0f && same(*event.render, opened_render_) && tracker_.predict(event.frame);
    auto result = replacement_.apply(context, *event.render, wanted);
    replaced_ = result == ReplacementResult::Replaced;
    if (wanted && !replaced_) note(std::string("resolve not replaced: ") + replacement_name(result));
    return replaced_ ? temporal::DrawDecision::Skip : temporal::DrawDecision::Proceed;
}

bool UpscaleStage::ensure_session(temporal::RenderSize render, temporal::RenderSize output) noexcept {
    if (opened_ && same(render, opened_render_) && same(output, opened_output_)) return ready_;
    if (opened_) session_.close();
    opened_ = true;
    opened_render_ = render;
    opened_output_ = output;
    pending_reset_ = true;
    auto status = session_.open({render, output, true, true});
    ready_ = status == UpscaleStatus::Ready;
    note(std::string("session ") + (ready_ ? "active" : "inactive") + " status=" + status_name(status) +
        " reason=" + reason_name(session_.report().reason) + " provider=" + session_.report().active_version);
    return ready_;
}

FrameContract UpscaleStage::contract_for(const temporal::PassEvent& event, temporal::RenderSize output,
    temporal::Jitter jitter) noexcept {
    auto now = Clock::now();
    float elapsed = std::chrono::duration<float, std::milli>(now - last_output_).count();
    last_output_ = now;
    FrameContract contract;
    contract.render = *event.render;
    contract.output = output;
    contract.jitter = jitter;
    contract.frame_time_ms = elapsed > 0.5f && elapsed < 200.0f ? elapsed : default_frame_ms;
    contract.camera = {near_plane, far_plane, vertical_fov_};
    contract.reset = pending_reset_;
    return contract;
}

temporal::DrawDecision UpscaleStage::on_output(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept {
    auto found = bindings(context);
    if (!event.render || !found || !same(size_of(found->color.Get()), *event.render)) return temporal::DrawDecision::Proceed;
    auto output = size_of(found->backbuffer.Get());
    if (!ensure_session(*event.render, output) || !replaced_ || !event.depth || !event.resolve)
        return temporal::DrawDecision::Proceed;
    auto jitter = tracker_.predict(event.frame);
    if (!jitter) return temporal::DrawDecision::Proceed;
    auto resources = inputs_.prepare(context, {found->color.Get(), event.depth.Get(), event.resolve.Get(), *event.render, output});
    if (!resources) return temporal::DrawDecision::Proceed;
    auto status = session_.process(contract_for(event, output, *jitter), *resources);
    if (status != UpscaleStatus::Ready) {
        note(std::string("frame not upscaled: ") + status_name(status));
        if (session_.report().state == SessionState::Failed) ready_ = false;
        return temporal::DrawDecision::Proceed;
    }
    pending_reset_ = false;
    ContextState saved(context);
    blit_.render(context, inputs_.output_view(), found->target.Get(), output);
    return temporal::DrawDecision::Skip;
}
}
