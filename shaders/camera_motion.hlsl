#include "camera_frame.hlsl"

Texture2D<float> depth_texture : register(t0);
StructuredBuffer<CameraFrame> camera_frame : register(t1);

cbuffer MotionParameters : register(b0) {
    float2 render_size;
    float cabin_depth;
    float parameters_padding;
};

float4 MotionMain(float4 position : SV_Position) : SV_Target {
    CameraFrame camera = camera_frame[0];
    if (camera.valid == 0)
        return float4(0.0, 0.0, 0.0, 0.0);
    float depth = depth_texture.Load(int3(position.xy, 0));
    if (depth >= cabin_depth)
        return float4(0.0, 0.0, 0.0, 0.0);
    float2 current = position.xy - camera.jitter;
    float2 ndc = float2(current.x / render_size.x * 2.0 - 1.0, 1.0 - current.y / render_size.y * 2.0);
    float4 clip = mul(camera.reprojection, float4(ndc, depth, 1.0));
    if (clip.w < 1e-6)
        return float4(0.0, 0.0, 0.0, 0.0);
    float2 previous_ndc = clip.xy / clip.w;
    float2 previous = float2((previous_ndc.x + 1.0) * 0.5 * render_size.x, (1.0 - previous_ndc.y) * 0.5 * render_size.y);
    return float4(previous - current, 0.0, 0.0);
}
