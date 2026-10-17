#include "renderer/renderer.hpp"
#include "renderer/screen_space_constants.hpp"
#include "dxgi/depth_capture.hpp"

namespace neuralfx {
void Renderer::render_processed() {
    auto depth = config_.mode == TestMode::Raytracing ? DepthCapture::instance().take_view() : nullptr;
    if (!depth || !render_screen_space(depth.Get())) {
        render_photoreal(resources_.input(), input_encoding());
        return;
    }
    if (config_.raytracing.debug_view != TraceDebugView::Off) {
        draw(copy_pixel_.Get(), resources_.traced_input(), resources_.output());
        return;
    }
    render_photoreal(resources_.traced_input(), ColorEncoding::Linear);
}

bool Renderer::render_screen_space(ID3D11ShaderResourceView* depth) {
    auto constants = make_screen_space_constants(config_.raytracing,
        resources_.width(), resources_.height(), input_encoding());
    if (!constants) return false;
    screen_space_.render(context_.Get(), {resources_.input(), depth, resources_.traced_output(),
        resources_.width(), resources_.height()}, *constants);
    return true;
}

ColorEncoding Renderer::input_encoding() const noexcept {
    return resources_.needs_srgb_conversion() ? ColorEncoding::Srgb : ColorEncoding::Linear;
}
}
