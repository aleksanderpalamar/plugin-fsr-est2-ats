#include "d3d12_readback.hpp"

#include <cstring>

namespace spike {
namespace {
ComPtr<ID3D12Resource> create_buffer(ID3D12Device* device, UINT64 size) {
    D3D12_HEAP_PROPERTIES properties{D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN, 0, 0};
    D3D12_RESOURCE_DESC buffer{D3D12_RESOURCE_DIMENSION_BUFFER, 0, size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE};
    ComPtr<ID3D12Resource> resource;
    device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&resource));
    return resource;
}

void record_copy(D3D12Sink& sink, ID3D12Resource* texture, ID3D12Resource* buffer,
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint, D3D12_RESOURCE_STATES state) {
    D3D12_TEXTURE_COPY_LOCATION source{};
    source.pResource = texture;
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION destination{};
    destination.pResource = buffer;
    destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    destination.PlacedFootprint = footprint;
    begin_timing(sink);
    transition(sink, texture, state, D3D12_RESOURCE_STATE_COPY_SOURCE);
    sink.list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    transition(sink, texture, D3D12_RESOURCE_STATE_COPY_SOURCE, state);
    end_timing(sink);
}

std::optional<std::vector<unsigned char>> compact(ID3D12Resource* buffer,
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint, UINT rows, UINT64 row_size) {
    unsigned char* mapped = nullptr;
    if (FAILED(buffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped)))) return std::nullopt;
    std::vector<unsigned char> pixels(static_cast<size_t>(row_size) * rows);
    for (UINT row = 0; row < rows; ++row)
        std::memcpy(pixels.data() + row * row_size, mapped + footprint.Offset + row * footprint.Footprint.RowPitch,
            static_cast<size_t>(row_size));
    D3D12_RANGE none{0, 0};
    buffer->Unmap(0, &none);
    return pixels;
}
}

std::optional<Readback> read_texture(D3D12Sink& sink, ID3D12Resource* texture,
    D3D12_RESOURCE_STATES state, UINT64 wait_value) {
    auto description = texture->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT rows = 0;
    UINT64 row_size = 0;
    UINT64 total = 0;
    sink.device->GetCopyableFootprints(&description, 0, 1, 0, &footprint, &rows, &row_size, &total);
    auto buffer = create_buffer(sink.device.Get(), total);
    if (!buffer) return std::nullopt;
    record_copy(sink, texture, buffer.Get(), footprint, state);
    if (!submit(sink, wait_value)) return std::nullopt;
    auto pixels = compact(buffer.Get(), footprint, rows, row_size);
    if (!pixels) return std::nullopt;
    return Readback{std::move(*pixels), elapsed_ms(sink)};
}
}
