#include "renderer/gpu_profiler.hpp"
#include "common/logger.hpp"

#include <string>

namespace neuralfx {
HRESULT GpuProfiler::initialize(ID3D11Device* device) noexcept {
    reset();
    if (!device) return E_POINTER;
    for (auto& slot : slots_) {
        D3D11_QUERY_DESC description{D3D11_QUERY_TIMESTAMP_DISJOINT, 0};
        HRESULT result = device->CreateQuery(&description, &slot.disjoint);
        if (FAILED(result)) return result;
        description.Query = D3D11_QUERY_TIMESTAMP;
        result = device->CreateQuery(&description, &slot.start);
        if (FAILED(result)) return result;
        result = device->CreateQuery(&description, &slot.finish);
        if (FAILED(result)) return result;
    }
    ready_ = true;
    return S_OK;
}

void GpuProfiler::begin(ID3D11DeviceContext* context) noexcept {
    if (!ready_ || !context) return;
    Slot& slot = slots_[current_];
    if (slot.pending) return;
    context->Begin(slot.disjoint.Get());
    context->End(slot.start.Get());
    running_ = true;
}

void GpuProfiler::end(ID3D11DeviceContext* context) noexcept {
    if (!running_ || !context) return;
    Slot& slot = slots_[current_];
    context->End(slot.finish.Get());
    context->End(slot.disjoint.Get());
    slot.pending = true;
    running_ = false;
    current_ = (current_ + 1) % 3;
}

void GpuProfiler::poll(ID3D11DeviceContext* context) noexcept {
    if (!ready_ || !context) return;
    for (auto& slot : slots_) {
        if (!slot.pending) continue;
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data{};
        UINT64 start = 0;
        UINT64 finish = 0;
        constexpr UINT flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;
        if (context->GetData(slot.disjoint.Get(), &data, sizeof(data), flags) != S_OK) continue;
        if (context->GetData(slot.start.Get(), &start, sizeof(start), flags) != S_OK) continue;
        if (context->GetData(slot.finish.Get(), &finish, sizeof(finish), flags) != S_OK) continue;
        slot.pending = false;
        if (data.Disjoint || data.Frequency == 0 || finish < start) continue;
        double elapsed = static_cast<double>(finish - start) * 1000.0 / static_cast<double>(data.Frequency);
        average_ms_ = samples_ == 0 ? elapsed : 0.9 * average_ms_ + 0.1 * elapsed;
        ++samples_;
        if (samples_ % 120 == 0) log("GPU plugin average: " + std::to_string(average_ms_) + " ms");
    }
}

void GpuProfiler::reset() noexcept {
    for (auto& slot : slots_) {
        slot.disjoint.Reset();
        slot.start.Reset();
        slot.finish.Reset();
        slot.pending = false;
    }
    current_ = 0;
    samples_ = 0;
    running_ = false;
    ready_ = false;
    average_ms_ = 0;
}
}
