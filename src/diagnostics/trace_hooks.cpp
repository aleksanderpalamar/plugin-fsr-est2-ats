#include "diagnostics/trace_hooks.hpp"
#include "diagnostics/frame_tracer.hpp"
#include "diagnostics/trace_constants.hpp"
#include "diagnostics/trace_resources.hpp"
#include "dxgi/vtable_patch.hpp"

#include <type_traits>

namespace neuralfx::diagnostics {
namespace {
template <typename First, typename... Rest>
std::uint32_t element_count(First first, Rest...) noexcept {
    if constexpr (std::is_same_v<First, UINT>) return first;
    return 0;
}

inline std::uint32_t element_count() noexcept {
    return 0;
}

void record_draw(ID3D11DeviceContext* context, std::uint32_t count) noexcept {
    record_pipeline([&](FrameTrace& trace, PipelineState& state) {
        if (trace.wants_constants())
            for (auto& snapshot : read_constants(context, ShaderStage::Vertex, 4, 64)) trace.constants(std::move(snapshot));
        if (trace.wants_draw_detail()) trace.detail({count, state.pixel_shader(), state.inputs()});
        trace.draw();
    });
}

template <size_t Index, typename... Arguments>
struct DrawHook {
    using Function = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, Arguments...);
    static inline Function original = nullptr;

    static void STDMETHODCALLTYPE call(ID3D11DeviceContext* context, Arguments... arguments) noexcept {
        record_draw(context, element_count(arguments...));
        original(context, arguments...);
    }

    static bool install(ID3D11DeviceContext* context) noexcept {
        return patch_vtable<Function>(context, Index, &call, original);
    }
};

using Draw = DrawHook<13, UINT, UINT>;
using DrawIndexed = DrawHook<12, UINT, UINT, INT>;
using DrawIndexedInstanced = DrawHook<20, UINT, UINT, UINT, INT, UINT>;
using DrawInstanced = DrawHook<21, UINT, UINT, UINT, UINT>;
using DrawAuto = DrawHook<38>;
using DrawIndexedIndirect = DrawHook<39, ID3D11Buffer*, UINT>;
using DrawInstancedIndirect = DrawHook<40, ID3D11Buffer*, UINT>;

using SetViewports = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, const D3D11_VIEWPORT*);
using CopyRegion = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT,
    UINT, UINT, UINT, ID3D11Resource*, UINT, const D3D11_BOX*);
using CopyWhole = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, ID3D11Resource*);
using Resolve = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT,
    ID3D11Resource*, UINT, DXGI_FORMAT);
using ClearColor = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11RenderTargetView*, const FLOAT*);
using ClearDepth = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11DepthStencilView*, UINT, FLOAT, UINT8);

SetViewports original_set_viewports = nullptr;
CopyRegion original_copy_region = nullptr;
CopyWhole original_copy_whole = nullptr;
Resolve original_resolve = nullptr;
ClearColor original_clear_color = nullptr;
ClearDepth original_clear_depth = nullptr;

void record_copy(ID3D11Resource* destination, ID3D11Resource* source) noexcept {
    record([&](FrameTrace& trace) {
        if (auto target = describe_resource(destination)) trace.copy(resource_id(source), *target);
    });
}

void STDMETHODCALLTYPE on_set_viewports(ID3D11DeviceContext* context, UINT count,
    const D3D11_VIEWPORT* viewports) noexcept {
    record([&](FrameTrace& trace) {
        if (count > 0 && viewports) trace.viewport({viewports[0].Width, viewports[0].Height});
    });
    original_set_viewports(context, count, viewports);
}

void STDMETHODCALLTYPE on_copy_region(ID3D11DeviceContext* context, ID3D11Resource* destination,
    UINT destination_index, UINT x, UINT y, UINT z, ID3D11Resource* source, UINT source_index,
    const D3D11_BOX* box) noexcept {
    record_copy(destination, source);
    original_copy_region(context, destination, destination_index, x, y, z, source, source_index, box);
}

void STDMETHODCALLTYPE on_copy_whole(ID3D11DeviceContext* context, ID3D11Resource* destination,
    ID3D11Resource* source) noexcept {
    record_copy(destination, source);
    original_copy_whole(context, destination, source);
}

void STDMETHODCALLTYPE on_resolve(ID3D11DeviceContext* context, ID3D11Resource* destination,
    UINT destination_index, ID3D11Resource* source, UINT source_index, DXGI_FORMAT format) noexcept {
    record_copy(destination, source);
    original_resolve(context, destination, destination_index, source, source_index, format);
}

void STDMETHODCALLTYPE on_clear_color(ID3D11DeviceContext* context, ID3D11RenderTargetView* view,
    const FLOAT* color) noexcept {
    record([&](FrameTrace& trace) { trace.clear_color(resource_id(view)); });
    original_clear_color(context, view, color);
}

void STDMETHODCALLTYPE on_clear_depth(ID3D11DeviceContext* context, ID3D11DepthStencilView* view,
    UINT flags, FLOAT depth, UINT8 stencil) noexcept {
    record([&](FrameTrace& trace) { trace.clear_depth(resource_id(view)); });
    original_clear_depth(context, view, flags, depth, stencil);
}

bool install_draws(ID3D11DeviceContext* context) noexcept {
    return Draw::install(context) && DrawIndexed::install(context) &&
        DrawIndexedInstanced::install(context) && DrawInstanced::install(context) &&
        DrawAuto::install(context) && DrawIndexedIndirect::install(context) &&
        DrawInstancedIndirect::install(context);
}

bool install_resources(ID3D11DeviceContext* context) noexcept {
    return patch_vtable<SetViewports>(context, 44, &on_set_viewports, original_set_viewports) &&
        patch_vtable<CopyRegion>(context, 46, &on_copy_region, original_copy_region) &&
        patch_vtable<CopyWhole>(context, 47, &on_copy_whole, original_copy_whole) &&
        patch_vtable<ClearColor>(context, 50, &on_clear_color, original_clear_color) &&
        patch_vtable<ClearDepth>(context, 53, &on_clear_depth, original_clear_depth) &&
        patch_vtable<Resolve>(context, 57, &on_resolve, original_resolve);
}
}

bool install_trace_hooks(ID3D11DeviceContext* context) noexcept {
    if (!context) return false;
    return install_draws(context) && install_resources(context) && install_state_hooks(context);
}
}
