#pragma once

#include "d3d12_sink.hpp"

namespace spike {
struct FsrInputs {
    ID3D12Resource* color = nullptr;
    ID3D12Resource* depth = nullptr;
    ID3D12Resource* motion = nullptr;
    UINT width = 0;
    UINT height = 0;
};

bool probe_fsr(D3D12Sink& sink, const FsrInputs& inputs, UINT64 wait_value);
}
