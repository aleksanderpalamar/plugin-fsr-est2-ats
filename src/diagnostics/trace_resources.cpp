#include "diagnostics/trace_resources.hpp"

#include <wrl/client.h>

namespace neuralfx::diagnostics {
namespace {
Microsoft::WRL::ComPtr<ID3D11Resource> resource_of(ID3D11View* view) noexcept {
    Microsoft::WRL::ComPtr<ID3D11Resource> resource;
    if (view) view->GetResource(&resource);
    return resource;
}
}

std::uint64_t resource_id(ID3D11Resource* resource) noexcept {
    return reinterpret_cast<std::uintptr_t>(resource);
}

std::uint64_t resource_id(ID3D11View* view) noexcept {
    return resource_id(resource_of(view).Get());
}

std::optional<TraceTarget> describe_resource(ID3D11Resource* resource, DXGI_FORMAT view_format) noexcept {
    if (!resource) return std::nullopt;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    if (FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture)))) return std::nullopt;
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    DXGI_FORMAT format = view_format == DXGI_FORMAT_UNKNOWN ? description.Format : view_format;
    return TraceTarget{resource_id(resource), description.Width, description.Height,
        static_cast<std::uint32_t>(format), description.SampleDesc.Count};
}

std::optional<TraceTarget> describe_view(ID3D11RenderTargetView* view) noexcept {
    if (!view) return std::nullopt;
    D3D11_RENDER_TARGET_VIEW_DESC description{};
    view->GetDesc(&description);
    return describe_resource(resource_of(view).Get(), description.Format);
}

std::optional<TraceTarget> describe_view(ID3D11DepthStencilView* view) noexcept {
    if (!view) return std::nullopt;
    D3D11_DEPTH_STENCIL_VIEW_DESC description{};
    view->GetDesc(&description);
    return describe_resource(resource_of(view).Get(), description.Format);
}
}
