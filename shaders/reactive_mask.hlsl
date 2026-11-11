Texture2D<float4> current_color : register(t0);
Texture2D<float4> previous_color : register(t1);
Texture2D<float2> motion_vectors : register(t2);

cbuffer ReactiveParameters : register(b0) {
    float2 render_size;
    float low_tolerance;
    float high_tolerance;
};

float ReactiveMain(float4 position : SV_Position) : SV_Target {
    int2 pixel = int2(position.xy);
    float3 current = current_color.Load(int3(pixel, 0)).rgb;
    float2 previous_position = position.xy + motion_vectors.Load(int3(pixel, 0));
    int2 base = int2(floor(previous_position));
    int2 limit = int2(render_size) - 1;
    float3 lowest = float3(1.0, 1.0, 1.0);
    float3 highest = float3(0.0, 0.0, 0.0);
    [unroll] for (int y = -1; y <= 1; ++y) {
        [unroll] for (int x = -1; x <= 1; ++x) {
            int2 texel = clamp(base + int2(x, y), int2(0, 0), limit);
            float3 sample_color = previous_color.Load(int3(texel, 0)).rgb;
            lowest = min(lowest, sample_color);
            highest = max(highest, sample_color);
        }
    }
    float3 outside = max(max(lowest - current, current - highest), float3(0.0, 0.0, 0.0));
    float distance_outside = max(outside.r, max(outside.g, outside.b));
    return saturate((distance_outside - low_tolerance) / (high_tolerance - low_tolerance));
}
