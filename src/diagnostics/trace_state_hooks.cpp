#include "diagnostics/trace_hooks.hpp"
#include "diagnostics/frame_tracer.hpp"
#include "diagnostics/trace_resources.hpp"
#include "dxgi/vtable_patch.hpp"

#include <vector>

namespace neuralfx::diagnostics {
namespace {
using SetResources = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT,
    ID3D11ShaderResourceView* const*);
using SetPixelShader = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11PixelShader*,
    ID3D11ClassInstance* const*, UINT);

SetResources original_set_resources = nullptr;
SetPixelShader original_set_pixel_shader = nullptr;

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
}

bool install_state_hooks(ID3D11DeviceContext* context) noexcept {
    return patch_vtable<SetResources>(context, 8, &on_set_resources, original_set_resources) &&
        patch_vtable<SetPixelShader>(context, 9, &on_set_pixel_shader, original_set_pixel_shader);
}
}
