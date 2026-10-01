float3 AcesFitted(float3 color) {
    float3 numerator = color * (2.51 * color + 0.03);
    float3 denominator = color * (2.43 * color + 0.59) + 0.14;
    return saturate(numerator / denominator);
}

float3 ReserveHighlightHeadroom(float3 color) {
    static const float headroom_scale = 0.45;
    float mask = smoothstep(0.55, 1.0, Luminance(color));
    return color * (1.0 - headroom_scale * tone_strength * mask);
}

float3 ApplyFilmicTonemap(float3 color) {
    float mask = smoothstep(0.35, 0.90, Luminance(color));
    float3 adjusted = ReserveHighlightHeadroom(color);
    float3 mapped = AcesFitted(adjusted) / AcesFitted(float3(1.0, 1.0, 1.0));
    return lerp(adjusted, mapped, tone_strength * mask);
}
