#include "temporal/resolve_capture.hpp"
#include "temporal/bound_constants.hpp"
#include "temporal/resolve_constants.hpp"

namespace neuralfx::temporal {
namespace {
constexpr UINT resolve_bytes = resolve_floats * sizeof(float);
}

ResolveCapture::ResolveCapture() noexcept : ring_(resolve_bytes) {}

bool ResolveCapture::capture(ID3D11DeviceContext* context, std::uint64_t frame) noexcept {
    auto bound = bound_constants(context, ConstantStage::Pixel, 0);
    if (!holds(bound, resolve_bytes)) return false;
    return ring_.copy(context, frame, bound.buffer.Get(), bound.offset_bytes);
}
}
