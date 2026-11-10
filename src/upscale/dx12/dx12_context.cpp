#include "upscale/dx12/dx12_context.hpp"

#include <dxgi1_4.h>
#include <new>

namespace neuralfx::upscale {
namespace {
constexpr DWORD wait_milliseconds = 10000;
}

Dx12Context::~Dx12Context() {
    if (event_) CloseHandle(event_);
}

std::unique_ptr<Dx12Context> Dx12Context::create(LUID adapter, HANDLE shared_fence) {
    std::unique_ptr<Dx12Context> context(new (std::nothrow) Dx12Context());
    if (context && context->initialize(adapter, shared_fence)) return context;
    return nullptr;
}

bool Dx12Context::initialize(LUID adapter, HANDLE shared_fence) noexcept {
    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgi_adapter;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return false;
    if (FAILED(factory->EnumAdapterByLuid(adapter, IID_PPV_ARGS(&dxgi_adapter)))) return false;
    if (FAILED(D3D12CreateDevice(dxgi_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_)))) return false;
    D3D12_COMMAND_QUEUE_DESC description{D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    if (FAILED(device_->CreateCommandQueue(&description, IID_PPV_ARGS(&queue_)))) return false;
    if (FAILED(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_)))) return false;
    if (FAILED(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator_.Get(), nullptr,
            IID_PPV_ARGS(&list_)))) return false;
    if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&local_fence_)))) return false;
    if (FAILED(device_->OpenSharedHandle(shared_fence, IID_PPV_ARGS(&shared_fence_)))) return false;
    event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return event_ != nullptr;
}

void Dx12Context::transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after) noexcept {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    list_->ResourceBarrier(1, &barrier);
}

bool Dx12Context::submit(std::uint64_t wait_value) noexcept {
    if (FAILED(list_->Close())) return false;
    if (wait_value > 0 && FAILED(queue_->Wait(shared_fence_.Get(), wait_value))) return false;
    ID3D12CommandList* lists[]{list_.Get()};
    queue_->ExecuteCommandLists(1, lists);
    ++local_value_;
    if (FAILED(queue_->Signal(local_fence_.Get(), local_value_))) return false;
    if (FAILED(local_fence_->SetEventOnCompletion(local_value_, event_))) return false;
    if (WaitForSingleObject(event_, wait_milliseconds) != WAIT_OBJECT_0) return false;
    return SUCCEEDED(allocator_->Reset()) && SUCCEEDED(list_->Reset(allocator_.Get(), nullptr));
}
}
