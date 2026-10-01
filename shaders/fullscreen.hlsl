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

float3 load_rgb(int2 position, int2 limit) {
    return source_texture.Load(int3(clamp(position, int2(0, 0), limit), 0)).rgb;
}

float4 RcasMain(VertexOutput input) : SV_Target {
    uint width;
    uint height;
    source_texture.GetDimensions(width, height);
    int2 position = int2(input.position.xy);
    int2 limit = int2(width - 1, height - 1);
    float4 center = source_texture.Load(int3(clamp(position, int2(0, 0), limit), 0));
    float3 north = load_rgb(position + int2(0, -1), limit);
    float3 west = load_rgb(position + int2(-1, 0), limit);
    float3 east = load_rgb(position + int2(1, 0), limit);
    float3 south = load_rgb(position + int2(0, 1), limit);
    float3 ring_min = min(min(north, west), min(east, south));
    float3 ring_max = max(max(north, west), max(east, south));
    float3 lower_limit = min(ring_min, center.rgb) / max(4.0 * ring_max, 0.00001);
    float3 upper_limit = (1.0 - max(ring_max, center.rgb)) / min(4.0 * ring_min - 4.0, -0.00001);
    float3 channel_lobe = max(-lower_limit, upper_limit);
    float lobe = clamp(max(channel_lobe.r, max(channel_lobe.g, channel_lobe.b)), -0.1875, 0.0) * amount;
    float3 resolved = (center.rgb + lobe * (north + west + east + south)) / (1.0 + 4.0 * lobe);
    return float4(saturate(resolved), center.a);
}
