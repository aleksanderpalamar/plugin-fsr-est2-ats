#include "upscale/resolve_replacement.hpp"

#include <wrl/client.h>
#include <array>
#include <utility>
#include <vector>

namespace neuralfx::upscale {
namespace {
using Microsoft::WRL::ComPtr;

template <typename View>
ComPtr<ID3D11Texture2D> texture_of(View* view) {
    ComPtr<ID3D11Resource> resource;
    ComPtr<ID3D11Texture2D> texture;
    if (view) view->GetResource(&resource);
    if (resource) resource.As(&texture);
    return texture;
}

bool candidate(ID3D11Texture2D* texture, const D3D11_TEXTURE2D_DESC& target, temporal::RenderSize render,
    std::initializer_list<std::uintptr_t> excluded) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    if (description.Format != target.Format || description.Width != render.width || description.Height != render.height)
        return false;
    auto address = reinterpret_cast<std::uintptr_t>(texture);
    for (auto other : excluded)
        if (other == address) return false;
    return true;
}

std::vector<ComPtr<ID3D11Texture2D>> current_colors(ID3D11DeviceContext* context, const D3D11_TEXTURE2D_DESC& target,
    temporal::RenderSize render, std::initializer_list<std::uintptr_t> excluded) {
    std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> views{};
    context->PSGetShaderResources(0, static_cast<UINT>(views.size()), views.data());
    std::vector<ComPtr<ID3D11Texture2D>> found;
    for (auto* raw : views) {
        ComPtr<ID3D11ShaderResourceView> view;
        view.Attach(raw);
        auto texture = texture_of(view.Get());
        if (texture && candidate(texture.Get(), target, render, excluded)) found.push_back(texture);
    }
    return found;
}
}

ReplacementResult ResolveReplacement::apply(ID3D11DeviceContext* context, temporal::RenderSize render,
    bool replace) noexcept {
    std::array<ID3D11RenderTargetView*, 2> raw{};
    context->OMGetRenderTargets(static_cast<UINT>(raw.size()), raw.data(), nullptr);
    ComPtr<ID3D11RenderTargetView> first_view;
    ComPtr<ID3D11RenderTargetView> second_view;
    first_view.Attach(raw[0]);
    second_view.Attach(raw[1]);
    auto first = texture_of(first_view.Get());
    auto second = texture_of(second_view.Get());
    if (!first || !second) return ReplacementResult::NoTargets;
    auto history = std::exchange(previous_history_, reinterpret_cast<std::uintptr_t>(second.Get()));
    if (!replace) return ReplacementResult::Tracked;
    D3D11_TEXTURE2D_DESC target{};
    D3D11_TEXTURE2D_DESC other{};
    first->GetDesc(&target);
    second->GetDesc(&other);
    if (target.Format != other.Format || target.Width != other.Width || target.Height != other.Height)
        return ReplacementResult::FormatMismatch;
    try {
        auto found = current_colors(context, target, render, {reinterpret_cast<std::uintptr_t>(first.Get()),
            reinterpret_cast<std::uintptr_t>(second.Get()), history});
        if (found.empty()) return ReplacementResult::NoCurrentColor;
        if (found.size() > 1) return ReplacementResult::Ambiguous;
        context->CopyResource(first.Get(), found.front().Get());
        context->CopyResource(second.Get(), found.front().Get());
    } catch (...) {
        return ReplacementResult::NoCurrentColor;
    }
    return ReplacementResult::Replaced;
}

const char* replacement_name(ReplacementResult result) noexcept {
    switch (result) {
    case ReplacementResult::Replaced: return "replaced";
    case ReplacementResult::Tracked: return "tracked";
    case ReplacementResult::NoTargets: return "no-targets";
    case ReplacementResult::NoCurrentColor: return "no-current-color";
    case ReplacementResult::Ambiguous: return "ambiguous";
    case ReplacementResult::FormatMismatch: return "format-mismatch";
    }
    return "unknown";
}
}
