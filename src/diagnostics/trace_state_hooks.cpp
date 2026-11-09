#include "diagnostics/trace_hooks.hpp"
#include "diagnostics/frame_tracer.hpp"
#include "diagnostics/trace_resources.hpp"
#include "dxgi/vtable_patch.hpp"

#include <unordered_map>
#include <vector>

namespace neuralfx::diagnostics {
namespace {
using SetResources = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT,
    ID3D11ShaderResourceView* const*);
using SetPixelShader = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11PixelShader*,
    ID3D11ClassInstance* const*, UINT);
using SetConstants = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, ID3D11Buffer* const*);
using Map = HRESULT(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT,
    D3D11_MAPPED_SUBRESOURCE*);
using Unmap = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT);
using Update = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, const D3D11_BOX*,
    const void*, UINT, UINT);

struct Mapping {
    const void* data = nullptr;
    std::uint32_t size = 0;
};

SetResources original_set_resources = nullptr;
SetPixelShader original_set_pixel_shader = nullptr;
SetConstants original_set_constants = nullptr;
Map original_map = nullptr;
Unmap original_unmap = nullptr;
Update original_update = nullptr;
std::unordered_map<std::uint64_t, Mapping> mappings;

void STDMETHODCALLTYPE on_set_resources(ID3D11DeviceContext* context, UINT start, UINT count,
    ID3D11ShaderResourceView* const* views) noexcept {
    record_pipeline([&](FrameTrace& trace, PipelineState& state) {
        std::vector<std::uint64_t> ids(count, 0);
        for (UINT index = 0; views && index < count; ++index) {
            auto input = views[index] ? describe_input(views[index]) : std::nullopt;
            if (!input) continue;
            trace.read(input->target, input->role);
            ids[index] = input->target.id;
        }
        state.set_resources(start, ids);
    });
    original_set_resources(context, start, count, views);
}

void STDMETHODCALLTYPE on_set_pixel_shader(ID3D11DeviceContext* context, ID3D11PixelShader* shader,
    ID3D11ClassInstance* const* instances, UINT count) noexcept {
    record_pipeline([&](FrameTrace&, PipelineState& state) {
        state.set_pixel_shader(reinterpret_cast<std::uintptr_t>(shader));
    });
    original_set_pixel_shader(context, shader, instances, count);
}

void STDMETHODCALLTYPE on_set_constants(ID3D11DeviceContext* context, UINT start, UINT count,
    ID3D11Buffer* const* buffers) noexcept {
    record_pipeline([&](FrameTrace&, PipelineState& state) {
        std::vector<std::uint64_t> ids(count, 0);
        for (UINT index = 0; buffers && index < count; ++index) ids[index] = resource_id(buffers[index]);
        state.set_constant_buffers(start, ids);
    });
    original_set_constants(context, start, count, buffers);
}

HRESULT STDMETHODCALLTYPE on_map(ID3D11DeviceContext* context, ID3D11Resource* resource, UINT index,
    D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE* mapped) noexcept {
    HRESULT result = original_map(context, resource, index, type, flags, mapped);
    if (FAILED(result) || !mapped) return result;
    record_pipeline([&](FrameTrace&, PipelineState&) {
        if (auto size = constant_buffer_size(resource)) mappings[resource_id(resource)] = {mapped->pData, *size};
    });
    return result;
}

void STDMETHODCALLTYPE on_unmap(ID3D11DeviceContext* context, ID3D11Resource* resource, UINT index) noexcept {
    record_pipeline([&](FrameTrace&, PipelineState& state) {
        auto found = mappings.find(resource_id(resource));
        if (found == mappings.end()) return;
        state.store_constants(found->first, found->second.data, found->second.size);
        mappings.erase(found);
    });
    original_unmap(context, resource, index);
}

void STDMETHODCALLTYPE on_update(ID3D11DeviceContext* context, ID3D11Resource* resource, UINT index,
    const D3D11_BOX* box, const void* data, UINT row, UINT depth) noexcept {
    record_pipeline([&](FrameTrace&, PipelineState& state) {
        auto size = box ? std::nullopt : constant_buffer_size(resource);
        if (size) state.store_constants(resource_id(resource), data, *size);
    });
    original_update(context, resource, index, box, data, row, depth);
}
}

bool install_state_hooks(ID3D11DeviceContext* context) noexcept {
    return patch_vtable<SetConstants>(context, 7, &on_set_constants, original_set_constants) &&
        patch_vtable<SetResources>(context, 8, &on_set_resources, original_set_resources) &&
        patch_vtable<SetPixelShader>(context, 9, &on_set_pixel_shader, original_set_pixel_shader) &&
        patch_vtable<Map>(context, 14, &on_map, original_map) &&
        patch_vtable<Unmap>(context, 15, &on_unmap, original_unmap) &&
        patch_vtable<Update>(context, 48, &on_update, original_update);
}
}
