Texture2D scene_color : register(t0);
Texture2D<float> scene_depth : register(t1);

cbuffer ScreenSpaceParameters : register(b0) {
    float2 inverse_size;
    float strength;
    float frame_index;
};

struct PixelInput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float DepthAt(float2 uv) {
    uint width;
    uint height;
    scene_depth.GetDimensions(width, height);
    int2 pixel = clamp(int2(uv * float2(width, height)), int2(0, 0), int2(width - 1, height - 1));
    return scene_depth.Load(int3(pixel, 0));
}

float3 ColorAt(float2 uv) {
    uint width;
    uint height;
    scene_color.GetDimensions(width, height);
    int2 pixel = clamp(int2(uv * float2(width, height)), int2(0, 0), int2(width - 1, height - 1));
    return scene_color.Load(int3(pixel, 0)).rgb;
}

float2 SurfaceSlope(float2 uv) {
    float left = DepthAt(uv - float2(inverse_size.x, 0));
    float right = DepthAt(uv + float2(inverse_size.x, 0));
    float top = DepthAt(uv - float2(0, inverse_size.y));
    float bottom = DepthAt(uv + float2(0, inverse_size.y));
    return float2(right - left, bottom - top);
}

float3 TraceVisible(float2 uv, float depth, float2 direction, out float hit) {
    hit = 0;
    float2 ray = normalize(direction);
    float3 color = 0;
    [loop]
    for (int step_index = 1; step_index <= 12; ++step_index) {
        float distance = float(step_index * step_index) * 2.0;
        float2 sample_uv = uv + ray * inverse_size * distance;
        if (any(sample_uv <= 0) || any(sample_uv >= 1)) break;
        float sample_depth = DepthAt(sample_uv);
        float thickness = max(0.0001, (1.0 - depth) * 0.18);
        if (sample_depth >= depth - 0.0001 || sample_depth < depth - thickness) continue;
        color = ColorAt(sample_uv);
        hit = 1.0 - distance / 300.0;
        break;
    }
    return color;
}

float4 TraceMain(PixelInput input) : SV_Target {
    float4 original = scene_color.Load(int3(int2(input.position.xy), 0));
    float depth = DepthAt(input.uv);
    if (depth >= 0.9999 || strength <= 0) return original;

    float2 slope = SurfaceSlope(input.uv);
    float3 normal = normalize(float3(-slope * 80.0, 1.0));
    float3 view = normalize(float3(input.uv * 2.0 - 1.0, -1.0));
    float2 reflected = reflect(view, normal).xy;
    float reflection_hit = 0;
    float3 reflection = TraceVisible(input.uv, depth, reflected + float2(0.0001, 0), reflection_hit);
    float facing = saturate(dot(normal, -view));
    float fresnel = pow(1.0 - facing, 5.0);

    float3 bounce = 0;
    float occlusion = 0;
    [unroll]
    for (int ray_index = 0; ray_index < 4; ++ray_index) {
        float angle = (float(ray_index) + frac(frame_index * 0.618)) * 1.5707963;
        float ray_hit = 0;
        bounce += TraceVisible(input.uv, depth, float2(cos(angle), sin(angle)), ray_hit) * ray_hit;
        occlusion += ray_hit;
    }
    float3 color = original.rgb * (1.0 - strength * occlusion * 0.025);
    color += bounce * strength * 0.025;
    color = lerp(color, reflection, saturate(strength * fresnel * reflection_hit * 0.25));
    return float4(saturate(color), original.a);
}
