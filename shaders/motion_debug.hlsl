Texture2D<float2> motion_texture : register(t0);

cbuffer OverlayParameters : register(b0) {
    float2 overlay_size;
    float motion_scale;
    float padding;
};

float4 OverlayMain(float4 position : SV_Position) : SV_Target {
    uint width;
    uint height;
    motion_texture.GetDimensions(width, height);
    int2 texel = int2(position.xy / overlay_size * float2(width, height));
    texel = clamp(texel, int2(0, 0), int2(width - 1, height - 1));
    float2 motion = motion_texture.Load(int3(texel, 0));
    return float4(saturate(0.5 + motion * motion_scale), 0.5, 1.0);
}
