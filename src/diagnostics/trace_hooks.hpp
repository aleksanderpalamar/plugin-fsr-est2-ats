#pragma once

#include <d3d11.h>

namespace neuralfx::diagnostics {
bool install_trace_hooks(ID3D11DeviceContext* context) noexcept;
bool install_state_hooks(ID3D11DeviceContext* context) noexcept;
}
