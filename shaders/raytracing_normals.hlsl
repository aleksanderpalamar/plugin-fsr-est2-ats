float3 NearestDifference(float3 center, float3 before, float3 after) {
    float3 forward = after - center;
    float3 backward = center - before;
    if (dot(backward, backward) == 0.0) return forward;
    if (dot(forward, forward) == 0.0) return backward;
    return abs(forward.z) < abs(backward.z) ? forward : backward;
}

float3 ViewNormal(int2 pixel, float3 center) {
    float3 horizontal = NearestDifference(center,
        ViewPositionAt(pixel - int2(1, 0)), ViewPositionAt(pixel + int2(1, 0)));
    float3 vertical = NearestDifference(center,
        ViewPositionAt(pixel - int2(0, 1)), ViewPositionAt(pixel + int2(0, 1)));
    float3 normal = cross(horizontal, vertical);
    float length_squared = dot(normal, normal);
    if (length_squared <= 1e-20) return float3(0.0, 0.0, -1.0);
    normal *= rsqrt(length_squared);
    return dot(normal, center) > 0.0 ? -normal : normal;
}
