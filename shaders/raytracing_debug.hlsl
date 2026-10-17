static const uint debug_off = 0;
static const uint debug_depth = 1;
static const uint debug_normals = 2;
static const uint debug_occlusion = 3;

float3 DebugColor(int2 pixel, float view_depth, bool skipped) {
    if (debug_view == debug_depth) return saturate(view_depth / max_distance);
    if (skipped) return 0.0;
    float3 position = ViewPosition(PixelUv(pixel), view_depth);
    float3 normal = ViewNormal(pixel, position);
    if (debug_view == debug_normals) return normal * 0.5 + 0.5;
    if (debug_view == debug_occlusion) return 1.0 - SampleOcclusion(pixel, position, normal).amount;
    Reflection reflection = TraceReflection(pixel, position, normal);
    return LinearToSrgb(reflection.color * reflection.weight);
}
