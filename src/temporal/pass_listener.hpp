#pragma once

#include "temporal/scene_recognizer.hpp"

#include <d3d11.h>
#include <cstdint>
#include <optional>
#include <wrl/client.h>

namespace neuralfx::temporal {
enum class DrawDecision { Proceed, Skip };

struct PassEvent {
    std::uint64_t frame = 0;
    PassRole role = PassRole::Other;
    std::optional<RenderSize> render;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth;
    Microsoft::WRL::ComPtr<ID3D11Buffer> resolve;
};

class PassListener {
public:
    virtual ~PassListener() = default;
    virtual DrawDecision first_draw(ID3D11DeviceContext* context, const PassEvent& event) noexcept = 0;
};
}
