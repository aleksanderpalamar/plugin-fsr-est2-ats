#include "temporal/scene_tracker.hpp"
#include "dxgi/vtable_patch.hpp"

namespace neuralfx::temporal {
namespace {
template <size_t Index, typename... Arguments>
struct DrawHook {
    using Function = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, Arguments...);
    static inline Function original = nullptr;

    static void STDMETHODCALLTYPE call(ID3D11DeviceContext* context, Arguments... arguments) noexcept {
        if (scene_tracker().draw(context) == DrawDecision::Skip) return;
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
}

bool install_scene_hooks(ID3D11DeviceContext* context) noexcept {
    if (!context) return false;
    bool draws = Draw::install(context) && DrawIndexed::install(context);
    return draws && DrawIndexedInstanced::install(context) && DrawInstanced::install(context);
}

void scene_targets_bound(ID3D11DeviceContext* context) noexcept {
    scene_tracker().targets_bound(context);
}
}
