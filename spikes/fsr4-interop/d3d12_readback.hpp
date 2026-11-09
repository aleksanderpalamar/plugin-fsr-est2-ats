#pragma once

#include "d3d12_sink.hpp"

#include <optional>
#include <vector>

namespace spike {
struct Readback {
    std::vector<unsigned char> pixels;
    double milliseconds = 0.0;
};

std::optional<Readback> read_texture(D3D12Sink& sink, ID3D12Resource* texture,
    D3D12_RESOURCE_STATES state, UINT64 wait_value);
}
