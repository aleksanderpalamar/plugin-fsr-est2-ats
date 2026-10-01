static const float clarity_activity_start = 0.005;
static const float clarity_activity_end = 0.06;
static const float clarity_detail_start = 0.002;
static const float clarity_detail_end = 0.03;
static const float clarity_detail_limit = 0.04;

float3 LoadClarityNeighbor(int2 position, int2 limit, float2 uv) {
    return ApplyNeural(LoadLinear(position, limit), uv);
}

float3 ApplyClarity(float3 center, float2 uv, int2 position, int2 limit, float2 texel) {
    if (clarity <= 0.0) return center;
    float3 north = LoadClarityNeighbor(position + int2(0, -1), limit, uv - float2(0.0, texel.y));
    float3 south = LoadClarityNeighbor(position + int2(0, 1), limit, uv + float2(0.0, texel.y));
    float3 east = LoadClarityNeighbor(position + int2(1, 0), limit, uv + float2(texel.x, 0.0));
    float3 west = LoadClarityNeighbor(position + int2(-1, 0), limit, uv - float2(texel.x, 0.0));
    float3 average = (center * 4.0 + north + south + east + west) * 0.125;
    float detail = Luminance(center) - Luminance(average);
    float activity = abs(Luminance(north) - Luminance(south))
        + abs(Luminance(east) - Luminance(west));
    float mask = smoothstep(clarity_activity_start, clarity_activity_end, activity)
        * smoothstep(clarity_detail_start, clarity_detail_end, abs(detail));
    float limited = clamp(detail, -clarity_detail_limit, clarity_detail_limit) * clarity * mask;
    return max(center + limited, 0.0);
}
