float3 ApplyLut(float3 color) {
    if (has_lut == 0 || lut_strength <= 0.0 || lut_size < 2.0) return color;
    float scale = (lut_size - 1.0) / lut_size;
    float offset = 0.5 / lut_size;
    float3 encoded = saturate(LinearToSrgb(color));
    float3 uvw = encoded * scale + offset;
    float3 graded = SrgbToLinear(lut_texture.SampleLevel(linear_sampler, uvw, 0).rgb);
    return lerp(color, graded, lut_strength);
}
