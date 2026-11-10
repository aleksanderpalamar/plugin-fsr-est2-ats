#include "temporal/staging_ring.hpp"

#include <cstring>
#include <utility>

namespace neuralfx::temporal {
bool StagingRing::copy(ID3D11DeviceContext* context, std::uint64_t frame, ID3D11Buffer* source,
    UINT offset_bytes) noexcept {
    auto& slot = pending_[frame % pending_.size()];
    if (!prepare(context, slot)) return false;
    D3D11_BOX box{offset_bytes, 0, 0, offset_bytes + bytes_, 1, 1};
    context->CopySubresourceRegion(slot.staging.Get(), 0, 0, 0, 0, source, 0, &box);
    slot.frame = frame;
    return true;
}

std::vector<FrameFloats> StagingRing::collect(ID3D11DeviceContext* context) noexcept {
    std::vector<FrameFloats> completed;
    while (auto* slot = oldest()) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT result = context->Map(slot->staging.Get(), 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);
        if (result == DXGI_ERROR_WAS_STILL_DRAWING) break;
        auto frame = *slot->frame;
        slot->frame.reset();
        if (FAILED(result)) continue;
        try {
            FrameFloats read{frame, std::vector<float>(bytes_ / sizeof(float))};
            std::memcpy(read.values.data(), mapped.pData, bytes_);
            context->Unmap(slot->staging.Get(), 0);
            completed.push_back(std::move(read));
        } catch (...) {
            context->Unmap(slot->staging.Get(), 0);
            break;
        }
    }
    return completed;
}

bool StagingRing::prepare(ID3D11DeviceContext* context, Pending& slot) noexcept {
    if (slot.staging) return true;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    context->GetDevice(&device);
    D3D11_BUFFER_DESC description{bytes_, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
    return device && SUCCEEDED(device->CreateBuffer(&description, nullptr, &slot.staging));
}

StagingRing::Pending* StagingRing::oldest() noexcept {
    Pending* found = nullptr;
    for (auto& slot : pending_)
        if (slot.frame && (!found || *slot.frame < *found->frame)) found = &slot;
    return found;
}
}
