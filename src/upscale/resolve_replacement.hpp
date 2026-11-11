#pragma once

#include "temporal/projection_jitter.hpp"

#include <d3d11.h>
#include <cstdint>

namespace neuralfx::upscale {
enum class ReplacementResult { Replaced, Tracked, NoTargets, NoCurrentColor, Ambiguous, FormatMismatch };

class ResolveReplacement {
public:
    ReplacementResult apply(ID3D11DeviceContext* context, temporal::RenderSize render, bool replace) noexcept;
    void reset() noexcept { previous_history_ = 0; }

private:
    std::uintptr_t previous_history_ = 0;
};

const char* replacement_name(ReplacementResult result) noexcept;
}
