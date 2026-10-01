Texture2D source_texture : register(t0);

cbuffer Parameters : register(b0) {
    float amount;
    float3 padding;
};

struct VertexOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VertexOutput VSMain(uint vertex_id : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.position = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
}

float4 CopyMain(VertexOutput input) : SV_Target {
    uint width;
    uint height;
    source_texture.GetDimensions(width, height);
    int2 position = clamp(int2(input.position.xy), int2(0, 0), int2(width - 1, height - 1));
    return source_texture.Load(int3(position, 0));
}

#include "rcas.hlsl"

float4 RcasMain(VertexOutput input) : SV_Target {
    return ResolveRcas(int2(input.position.xy), amount);
}
