#include "raytracing_common.hlsl"
#include "raytracing_normals.hlsl"
#include "raytracing_reflection.hlsl"

float4 RefineTowardMain(PixelInput input) : SV_Target {
    ScreenRay ray;
    ray.start = float2(0.2, 0.5);
    ray.delta = float2(0.6, 0.0);
    ray.start_inverse_depth = 0.1;
    ray.delta_inverse_depth = 0.1;
    float hit = RefineHit(ray, 0.4, 0.45);
    return float4(RayDepth(ray, hit), hit, 0.0, 1.0);
}

float4 RefineAwayMain(PixelInput input) : SV_Target {
    ScreenRay ray;
    ray.start = float2(0.2, 0.5);
    ray.delta = float2(0.6, 0.0);
    ray.start_inverse_depth = 0.2;
    ray.delta_inverse_depth = -0.1;
    float hit = RefineHit(ray, 0.55, 0.6);
    return float4(RayDepth(ray, hit), hit, 0.0, 1.0);
}

float4 ReflectionLinearMain(PixelInput input) : SV_Target {
    int2 pixel = int2(input.position.xy);
    float depth = DeviceDepthAt(pixel);
    if (IsBackground(depth)) return float4(0.0, 0.0, 0.0, 1.0);
    float3 position = ViewPosition(PixelUv(pixel), LinearDepth(depth));
    Reflection reflection = TraceReflection(pixel, position, ViewNormal(pixel, position));
    return float4(reflection.color * reflection.weight, 1.0);
}
