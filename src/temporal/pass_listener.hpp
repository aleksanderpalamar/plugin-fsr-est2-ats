#pragma once

#include "temporal/scene_recognizer.hpp"

#include <d3d11.h>
#include <cstdint>

namespace neuralfx::temporal {
class PassListener {
public:
    virtual ~PassListener() = default;
    virtual void first_draw(ID3D11DeviceContext* context, std::uint64_t frame, PassRole role) noexcept = 0;
};
}
