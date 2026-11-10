#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace neuralfx::temporal {
enum class ConstantStage { Vertex, Pixel };

struct BoundConstants {
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    UINT offset_bytes = 0;
    UINT range_bytes = 0;
};

BoundConstants bound_constants(ID3D11DeviceContext* context, ConstantStage stage, UINT slot) noexcept;
bool holds(const BoundConstants& bound, UINT bytes) noexcept;
}
