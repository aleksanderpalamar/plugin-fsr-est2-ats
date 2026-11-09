#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <optional>

namespace spike {
using Microsoft::WRL::ComPtr;

struct D3D12Sink {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> shared_fence;
    ComPtr<ID3D12Fence> local_fence;
    ComPtr<ID3D12QueryHeap> timestamps;
    ComPtr<ID3D12Resource> timestamp_readback;
    HANDLE event = nullptr;
    UINT64 local_value = 0;
    UINT64 frequency = 0;
};

std::optional<D3D12Sink> create_sink(LUID adapter, HANDLE fence_handle);
ComPtr<ID3D12Resource> open_texture(D3D12Sink& sink, HANDLE handle, const char* name);
void begin_timing(D3D12Sink& sink);
void end_timing(D3D12Sink& sink);
bool submit(D3D12Sink& sink, UINT64 wait_value);
double elapsed_ms(D3D12Sink& sink);
void transition(D3D12Sink& sink, ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after);
}
