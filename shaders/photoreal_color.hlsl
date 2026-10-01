static const float black_shadow_range = 0.25;
static const float black_toe_end = 0.03;
static const float contrast_pivot = 0.18;
static const float3 shadow_tint = float3(-0.03, 0.0, 0.04);
static const float3 highlight_tint = float3(0.04, 0.01, -0.04);

float3 ApplyExposure(float3 color) {
    return color * exp2(exposure_ev);
}

float3 ApplyBlackLevel(float3 color) {
    float luminance = Luminance(color);
    float shadow = 1.0 - saturate(luminance / black_shadow_range);
    float toe = smoothstep(0.0, black_toe_end, luminance);
    return max(color + black_level * shadow * toe, 0.0);
}

float3 ApplyContrast(float3 color) {
    float luminance = Luminance(color);
    float distance = (luminance - contrast_pivot) * luminance * (1.0 - saturate(luminance));
    float adjusted = max(0.0, luminance + (contrast - 1.0) * distance);
    return color * (adjusted / max(luminance, 0.00001));
}

float3 ShapeHighlights(float3 color) {
    float luminance = Luminance(color);
    float mask = smoothstep(highlight_start, highlight_end, luminance);
    float headroom = 1.0 - saturate(luminance);
    return color * (1.0 + highlight_boost * mask * headroom);
}

float3 ApplySplitToning(float3 color) {
    float luminance = Luminance(color);
    float shadow_mask = 1.0 - smoothstep(0.08, 0.48, luminance);
    float highlight_mask = smoothstep(0.40, 0.88, luminance);
    float3 cool = shadow_tint * shadow_coolness * shadow_mask;
    float3 warm = highlight_tint * highlight_warmth * highlight_mask;
    return max(color * (1.0 + cool + warm), 0.0);
}

float3 CompressSaturation(float3 color) {
    float luminance = Luminance(color);
    return lerp(luminance.xxx, color, saturation);
}
