float Noise(float2 position) {
    float2 seed = position + frame_index * float2(0.75487766, 0.56984029);
    return frac(52.9829189 * frac(dot(seed, float2(0.06711056, 0.00583715))));
}

float4 FinishMain(PixelInput input) : SV_Target {
    int2 position = int2(input.position.xy);
    float4 source = ResolveRcas(position, rcas_strength);
    float noise = Noise(input.position.xy) - 0.5;
    float dither = noise / 255.0;
    float3 color = saturate(source.rgb + noise * grain_strength + dither);
    if (srgb_target != 0) color = SrgbToLinear(color);
    return float4(color, source.a);
}
