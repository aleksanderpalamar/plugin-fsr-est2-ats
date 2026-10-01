#include "config/config.hpp"
#include "renderer/lut_data.hpp"
#include "renderer/photoreal_constants.hpp"

#include <sstream>

int main() {
    std::istringstream empty;
    auto defaults = neuralfx::parse_config(empty);
    if (defaults.enabled || defaults.mode != neuralfx::TestMode::Hook) return 1;
    std::istringstream valid(" enabled = true\nmode=rcas\nsharpness=0.8\n");
    auto configured = neuralfx::parse_config(valid);
    if (!configured.enabled || configured.mode != neuralfx::TestMode::Rcas) return 2;
    if (configured.sharpness < 0.79f || configured.sharpness > 0.81f) return 3;
    std::istringstream invalid("sharpness=nope\nmode=unknown\n");
    auto fallback = neuralfx::parse_config(invalid);
    if (fallback.sharpness != 0.65f || fallback.mode != neuralfx::TestMode::Hook) return 4;
    std::istringstream clamped("sharpness=2.5\n");
    if (neuralfx::parse_config(clamped).sharpness != 1.0f) return 5;
    if (defaults.photoreal.exposure_ev != 0.10f) return 6;
    if (defaults.photoreal.neural_strength != 0.0f) return 7;
    if (defaults.photoreal.photoreal_sharpness != 0.30f) return 8;
    auto disabled_optional = neuralfx::make_photoreal_constants(defaults, true, false, true, 2, 0.0f);
    if (disabled_optional.has_neural != 0 || disabled_optional.has_lut != 0) return 21;
    std::istringstream photoreal("mode=photoreal\nexposure_ev=0.25\nlut_strength=1\n"
        "grain_strength=0\nphotoreal_sharpness=0.3\nlut_path=grading.cube\n");
    auto photo_config = neuralfx::parse_config(photoreal);
    if (photo_config.mode != neuralfx::TestMode::Photoreal) return 9;
    if (photo_config.photoreal.exposure_ev != 0.25f || photo_config.lut_path != "grading.cube") return 10;
    auto constants = neuralfx::make_photoreal_constants(photo_config, true, false, false, 0, 0.0f);
    if (constants.decode_input != 1 || constants.srgb_target != 0) return 11;
    if (constants.has_neural != 0 || constants.has_lut != 0) return 12;
    if (constants.grain_strength != 0.0f) return 13;
    auto lut_constants = neuralfx::make_photoreal_constants(photo_config, false, true, true, 2, 1.0f);
    if (lut_constants.has_lut != 1 || lut_constants.has_neural != 0) return 14;
    std::istringstream identity("LUT_3D_INPUT_RANGE 0 1\nLUT_3D_SIZE 2\n"
        "0 0 0\n1.0001 0 0\n0 1 0\n1 1 0\n0 0 1\n1 0 1\n0 1 1\n1 1 1\n");
    auto lut = neuralfx::parse_lut(identity);
    if (!lut || lut->size != 2 || lut->texels.size() != 8) return 15;
    if (lut->texels[5][0] != 1.0f || lut->texels[5][2] != 1.0f) return 16;
    if (lut->texels[1][0] <= 1.0f) return 22;
    std::istringstream truncated("LUT_3D_SIZE 2\n0 0 0\n");
    if (neuralfx::parse_lut(truncated)) return 17;
    std::istringstream bad_domain("LUT_3D_SIZE 2\nDOMAIN_MAX 2 2 2\n");
    if (neuralfx::parse_lut(bad_domain)) return 18;
    std::istringstream bad_range("LUT_3D_INPUT_RANGE 0 2\nLUT_3D_SIZE 2\n");
    if (neuralfx::parse_lut(bad_range)) return 23;
    std::istringstream invalid_photo("exposure_ev=nan\nlut_strength=2\n"
        "highlight_start=1\nhighlight_end=0.5\n");
    auto sanitized = neuralfx::parse_config(invalid_photo);
    if (sanitized.photoreal.exposure_ev != 0.10f || sanitized.photoreal.lut_strength != 1.0f) return 19;
    if (sanitized.photoreal.highlight_start >= sanitized.photoreal.highlight_end) return 20;
    std::istringstream negative_black("black_level=-0.05\n");
    if (neuralfx::parse_config(negative_black).photoreal.black_level != 0.0f) return 24;
    return 0;
}
