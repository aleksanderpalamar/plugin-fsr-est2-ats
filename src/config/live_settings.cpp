#include "config/config.hpp"

namespace neuralfx {
void refresh_live_settings(Config& active, const Config& reloaded) noexcept {
    active.photoreal = reloaded.photoreal;
    active.raytracing = reloaded.raytracing;
    active.sharpness = reloaded.sharpness;
    active.lut_path = reloaded.lut_path;
}
}
