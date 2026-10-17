#include "color_space.hlsl"

Texture2D scene_color : register(t0);
Texture2D<float> scene_depth : register(t1);

cbuffer ScreenSpaceParameters : register(b0) {
    float2 size;
    float2 inverse_size;
    float tan_half_fov;
    float aspect;
    float near_plane;
    float far_plane;
    float max_distance;
    float thickness;
    float occlusion_radius;
    float reflection_strength;
    float occlusion_strength;
    float bounce_strength;
    uint depth_convention;
    uint debug_view;
    uint decode_input;
    uint3 padding;
};

struct PixelInput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

static const uint depth_reversed = 1;
static const float background_threshold = 0.999999;

int2 ClampPixel(int2 pixel) {
    return clamp(pixel, int2(0, 0), int2(size) - 1);
}

bool InsideScreen(int2 pixel) {
    return all(pixel >= 0) && all(pixel < int2(size));
}

float2 PixelUv(int2 pixel) {
    return (float2(pixel) + 0.5) * inverse_size;
}

int2 UvPixel(float2 uv) {
    return int2(floor(uv * size));
}

float StandardDepth(float device_depth) {
    return depth_convention == depth_reversed ? 1.0 - device_depth : device_depth;
}

bool IsBackground(float device_depth) {
    return StandardDepth(device_depth) >= background_threshold;
}

float LinearDepth(float device_depth) {
    float depth = StandardDepth(device_depth);
    return near_plane * far_plane / (far_plane - depth * (far_plane - near_plane));
}

float3 ViewPosition(float2 uv, float view_depth) {
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    return float3(ndc * float2(tan_half_fov * aspect, tan_half_fov), 1.0) * view_depth;
}

float2 ProjectUv(float3 position) {
    float2 ndc = position.xy / (position.z * float2(tan_half_fov * aspect, tan_half_fov));
    return float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
}

float DeviceDepthAt(int2 pixel) {
    return scene_depth.Load(int3(ClampPixel(pixel), 0));
}

float3 ViewPositionAt(int2 pixel) {
    int2 clamped = ClampPixel(pixel);
    return ViewPosition(PixelUv(clamped), LinearDepth(DeviceDepthAt(clamped)));
}

float3 DecodeColor(float3 color) {
    return decode_input != 0 ? SrgbToLinear(color) : color;
}

float3 SceneColorAt(int2 pixel) {
    return DecodeColor(scene_color.Load(int3(ClampPixel(pixel), 0)).rgb);
}

float InterleavedGradientNoise(float2 pixel) {
    return frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
}
