#include "raytracing_common.hlsl"
#include "raytracing_normals.hlsl"
#include "raytracing_occlusion.hlsl"
#include "raytracing_reflection.hlsl"
#include "raytracing_debug.hlsl"

bool HasEffects() {
    return reflection_strength > 0.0 || occlusion_strength > 0.0 || bounce_strength > 0.0;
}

float4 TraceMain(PixelInput input) : SV_Target {
    int2 pixel = int2(input.position.xy);
    float4 original = scene_color.Load(int3(ClampPixel(pixel), 0));
    float3 color = DecodeColor(original.rgb);
    float device_depth = DeviceDepthAt(pixel);
    float view_depth = LinearDepth(device_depth);
    bool skipped = IsBackground(device_depth) || view_depth >= max_distance;
    if (debug_view != debug_off) return float4(DebugColor(pixel, view_depth, skipped), original.a);
    if (skipped || !HasEffects()) return float4(color, original.a);
    float3 position = ViewPosition(PixelUv(pixel), view_depth);
    float3 normal = ViewNormal(pixel, position);
    color = ApplyOcclusion(color, pixel, position, normal);
    color = ApplyReflection(color, pixel, position, normal);
    return float4(saturate(color), original.a);
}
