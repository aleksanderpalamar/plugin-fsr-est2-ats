Texture2D<float> depth_texture : register(t0);

float DepthMain(float4 position : SV_Position) : SV_Target {
    return depth_texture.Load(int3(position.xy, 0));
}
