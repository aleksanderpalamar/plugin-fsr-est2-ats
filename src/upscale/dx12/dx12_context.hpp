#pragma once

#include <d3d12.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>
#include <windows.h>

namespace neuralfx::upscale {
class Dx12Context {
public:
    ~Dx12Context();
    Dx12Context() = default;
    Dx12Context(const Dx12Context&) = delete;
    Dx12Context& operator=(const Dx12Context&) = delete;

    static std::unique_ptr<Dx12Context> create(IDXGIAdapter* adapter, HANDLE shared_fence);
    ID3D12Device* device() const noexcept { return device_.Get(); }
    ID3D12GraphicsCommandList* list() const noexcept { return list_.Get(); }
    void transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) noexcept;
    bool submit(std::uint64_t wait_value) noexcept;

private:
    bool initialize(IDXGIAdapter* adapter, HANDLE shared_fence) noexcept;

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_;
    Microsoft::WRL::ComPtr<ID3D12Fence> shared_fence_;
    Microsoft::WRL::ComPtr<ID3D12Fence> local_fence_;
    HANDLE event_ = nullptr;
    std::uint64_t local_value_ = 0;
};
}
