#include "renderer/renderer.hpp"
#include "renderer/screen_space_constants.hpp"

namespace neuralfx {
void Renderer::render_screen_space(ID3D11ShaderResourceView* depth) {
    ScreenSpaceConstants settings{
        1.0f / static_cast<float>(resources_.width()),
        1.0f / static_cast<float>(resources_.height()),
        0.35f,
        frame_index_
    };
    context_->UpdateSubresource(trace_constants_.Get(), 0, nullptr, &settings, 0, 0);
    draw(trace_pixel_.Get(), resources_.input(), resources_.traced_output(),
        depth, nullptr, trace_constants_.Get());
}
}
