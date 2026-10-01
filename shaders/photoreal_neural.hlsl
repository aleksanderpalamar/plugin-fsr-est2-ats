float3 ApplyNeural(float3 color, float2 uv) {
    if (has_neural == 0 || neural_strength <= 0.0) return color;
    float4 neural = neural_texture.SampleLevel(linear_sampler, uv, 0);
    float confidence = saturate(neural.a);
    float3 residual = neural.rgb * 2.0 - 1.0;
    return max(color + residual * confidence * neural_strength, 0.0);
}
