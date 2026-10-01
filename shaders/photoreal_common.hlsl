Texture2D source_texture : register(t0);
Texture2D neural_texture : register(t1);
Texture3D lut_texture : register(t2);
SamplerState linear_sampler : register(s0);

cbuffer PhotorealParameters : register(b0) {
    float rcas_strength;
    float exposure_ev;
    float contrast;
    float saturation;
    float clarity;
    float highlight_boost;
    float highlight_warmth;
    float shadow_coolness;
    float neural_strength;
    float black_level;
    float lut_strength;
    float grain_strength;
    float tone_strength;
    float highlight_start;
    float highlight_end;
    float lut_size;
    uint decode_input;
    uint srgb_target;
    uint has_neural;
    uint has_lut;
    float frame_index;
    float3 padding;
};

struct PixelInput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

static const float3 luma_weights = float3(0.2126, 0.7152, 0.0722);

float Luminance(float3 color) {
    return dot(color, luma_weights);
}

float3 SrgbToLinear(float3 color) {
    float3 low = color / 12.92;
    float3 high = pow(max((color + 0.055) / 1.055, 0.0), 2.4);
    return lerp(high, low, step(color, 0.04045));
}

float3 LinearToSrgb(float3 color) {
    float3 low = color * 12.92;
    float3 high = 1.055 * pow(max(color, 0.0), 1.0 / 2.4) - 0.055;
    return lerp(high, low, step(color, 0.0031308));
}

float3 LoadLinear(int2 position, int2 limit) {
    float3 color = source_texture.Load(int3(clamp(position, int2(0, 0), limit), 0)).rgb;
    return decode_input != 0 ? SrgbToLinear(color) : color;
}
