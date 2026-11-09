#include "d3d12_sink.hpp"
#include "report.hpp"

#include <dxgi1_4.h>
#include <string>

namespace spike {
namespace {
std::string shader_model(ID3D12Device* device) {
    D3D12_FEATURE_DATA_SHADER_MODEL model{D3D_SHADER_MODEL_6_6};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &model, sizeof(model)))) return "unknown";
    return "0x" + std::to_string(model.HighestShaderModel);
}

HRESULT create_device(LUID luid, D3D12Sink& sink) {
    ComPtr<IDXGIFactory4> factory;
    HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    ComPtr<IDXGIAdapter1> adapter;
    if (SUCCEEDED(result)) result = factory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&adapter));
    if (SUCCEEDED(result)) result = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&sink.device));
    return result;
}

HRESULT create_queue(D3D12Sink& sink) {
    D3D12_COMMAND_QUEUE_DESC description{D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    HRESULT result = sink.device->CreateCommandQueue(&description, IID_PPV_ARGS(&sink.queue));
    if (SUCCEEDED(result))
        result = sink.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&sink.allocator));
    if (SUCCEEDED(result))
        result = sink.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, sink.allocator.Get(), nullptr,
            IID_PPV_ARGS(&sink.list));
    if (SUCCEEDED(result)) result = sink.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&sink.local_fence));
    if (SUCCEEDED(result)) result = sink.queue->GetTimestampFrequency(&sink.frequency);
    sink.event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return sink.event ? result : E_FAIL;
}

HRESULT create_timestamps(D3D12Sink& sink) {
    D3D12_QUERY_HEAP_DESC heap{D3D12_QUERY_HEAP_TYPE_TIMESTAMP, 2, 0};
    HRESULT result = sink.device->CreateQueryHeap(&heap, IID_PPV_ARGS(&sink.timestamps));
    D3D12_HEAP_PROPERTIES properties{D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN, 0, 0};
    D3D12_RESOURCE_DESC buffer{D3D12_RESOURCE_DIMENSION_BUFFER, 0, 16, 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE};
    if (SUCCEEDED(result))
        result = sink.device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&sink.timestamp_readback));
    return result;
}
}

std::optional<D3D12Sink> create_sink(LUID adapter, HANDLE fence_handle) {
    D3D12Sink sink;
    HRESULT result = create_device(adapter, sink);
    if (!report("d3d12.device", result, SUCCEEDED(result) ? "shader_model=" + shader_model(sink.device.Get()) : ""))
        return std::nullopt;
    if (!report("d3d12.queue", create_queue(sink))) return std::nullopt;
    if (!report("d3d12.timestamps", create_timestamps(sink))) return std::nullopt;
    result = sink.device->OpenSharedHandle(fence_handle, IID_PPV_ARGS(&sink.shared_fence));
    if (!report("d3d12.fence.open", result)) return std::nullopt;
    return sink;
}

ComPtr<ID3D12Resource> open_texture(D3D12Sink& sink, HANDLE handle, const char* name) {
    ComPtr<ID3D12Resource> resource;
    HRESULT result = sink.device->OpenSharedHandle(handle, IID_PPV_ARGS(&resource));
    std::string step = std::string("d3d12.open.") + name;
    std::string detail;
    if (SUCCEEDED(result)) {
        auto description = resource->GetDesc();
        detail = "format=" + std::to_string(description.Format) + " flags=0x" + std::to_string(description.Flags);
    }
    report(step.c_str(), result, detail);
    return resource;
}

void transition(D3D12Sink& sink, ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    sink.list->ResourceBarrier(1, &barrier);
}

void begin_timing(D3D12Sink& sink) {
    sink.list->EndQuery(sink.timestamps.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
}

void end_timing(D3D12Sink& sink) {
    sink.list->EndQuery(sink.timestamps.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    sink.list->ResolveQueryData(sink.timestamps.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2,
        sink.timestamp_readback.Get(), 0);
}

bool submit(D3D12Sink& sink, UINT64 wait_value) {
    if (FAILED(sink.list->Close())) return false;
    if (wait_value > 0 && FAILED(sink.queue->Wait(sink.shared_fence.Get(), wait_value))) return false;
    ID3D12CommandList* lists[]{sink.list.Get()};
    sink.queue->ExecuteCommandLists(1, lists);
    ++sink.local_value;
    if (FAILED(sink.queue->Signal(sink.local_fence.Get(), sink.local_value))) return false;
    if (FAILED(sink.local_fence->SetEventOnCompletion(sink.local_value, sink.event))) return false;
    if (WaitForSingleObject(sink.event, 10000) != WAIT_OBJECT_0) return false;
    return SUCCEEDED(sink.allocator->Reset()) && SUCCEEDED(sink.list->Reset(sink.allocator.Get(), nullptr));
}

double elapsed_ms(D3D12Sink& sink) {
    UINT64* ticks = nullptr;
    D3D12_RANGE range{0, 16};
    if (FAILED(sink.timestamp_readback->Map(0, &range, reinterpret_cast<void**>(&ticks)))) return -1.0;
    double milliseconds = static_cast<double>(ticks[1] - ticks[0]) * 1000.0 / static_cast<double>(sink.frequency);
    D3D12_RANGE none{0, 0};
    sink.timestamp_readback->Unmap(0, &none);
    return milliseconds;
}
}
