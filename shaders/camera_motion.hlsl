Texture2D<float> depth_texture : register(t0);

cbuffer MotionParameters : register(b0) {
    row_major float4x4 reprojection;
    float2 jitter;
    float2 render_size;
};

float4 MotionMain(float4 position : SV_Position) : SV_Target {
    float depth = depth_texture.Load(int3(position.xy, 0));
    float2 current = position.xy - jitter;
    float2 ndc = float2(current.x / render_size.x * 2.0 - 1.0, 1.0 - current.y / render_size.y * 2.0);
    float4 clip = mul(reprojection, float4(ndc, depth, 1.0));
    if (clip.w < 1e-6)
        return float4(0.0, 0.0, 0.0, 0.0);
    float2 previous_ndc = clip.xy / clip.w;
    float2 previous = float2((previous_ndc.x + 1.0) * 0.5 * render_size.x, (1.0 - previous_ndc.y) * 0.5 * render_size.y);
    return float4(previous - current, 0.0, 0.0);
}
