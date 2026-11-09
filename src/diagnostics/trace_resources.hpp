#pragma once

#include "diagnostics/frame_trace.hpp"

#include <d3d11.h>
#include <cstdint>
#include <optional>

namespace neuralfx::diagnostics {
std::uint64_t resource_id(ID3D11Resource* resource) noexcept;
std::uint64_t resource_id(ID3D11View* view) noexcept;
std::optional<TraceTarget> describe_resource(ID3D11Resource* resource,
    DXGI_FORMAT view_format = DXGI_FORMAT_UNKNOWN) noexcept;
std::optional<TraceTarget> describe_view(ID3D11RenderTargetView* view) noexcept;
std::optional<TraceTarget> describe_view(ID3D11DepthStencilView* view) noexcept;

struct TraceInput {
    TraceTarget target;
    TargetRole role = TargetRole::Color;
};

std::optional<TraceInput> describe_input(ID3D11ShaderResourceView* view) noexcept;
std::optional<std::uint32_t> constant_buffer_size(ID3D11Resource* resource) noexcept;
}
