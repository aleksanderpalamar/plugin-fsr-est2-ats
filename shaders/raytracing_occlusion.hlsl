static const uint occlusion_directions = 4;
static const uint occlusion_steps = 6;
static const float occlusion_bias = 0.1;
static const float occlusion_gain = 2.0;
static const float max_occlusion_pixels = 96.0;
static const float tau = 6.28318531;

struct Occlusion {
    float amount;
    float3 bounce;
};

float OcclusionRadiusPixels(float view_depth) {
    return occlusion_radius * size.y / (2.0 * tan_half_fov * view_depth);
}

Occlusion SampleOcclusion(int2 pixel, float3 position, float3 normal) {
    Occlusion result;
    result.amount = 0.0;
    result.bounce = 0.0;
    float radius_pixels = OcclusionRadiusPixels(position.z);
    if (radius_pixels < 1.0) return result;
    radius_pixels = min(radius_pixels, max_occlusion_pixels);
    float rotation = InterleavedGradientNoise(float2(pixel));
    float offset = InterleavedGradientNoise(float2(pixel.yx) + 17.0);
    float radius_squared = occlusion_radius * occlusion_radius;
    [loop]
    for (uint direction_index = 0; direction_index < occlusion_directions; ++direction_index) {
        float angle = (float(direction_index) + rotation) * tau / float(occlusion_directions);
        float2 direction = float2(cos(angle), sin(angle));
        [loop]
        for (uint step_index = 0; step_index < occlusion_steps; ++step_index) {
            float distance = (float(step_index) + offset) / float(occlusion_steps) * radius_pixels;
            int2 sample_pixel = pixel + int2(round(direction * max(distance, 1.0)));
            if (!InsideScreen(sample_pixel)) break;
            float3 offset_vector = ViewPositionAt(sample_pixel) - position;
            float distance_squared = dot(offset_vector, offset_vector);
            if (distance_squared <= 1e-8) continue;
            float cosine = dot(normal, offset_vector) * rsqrt(distance_squared);
            float falloff = saturate(1.0 - distance_squared / radius_squared);
            float weight = max(cosine - occlusion_bias, 0.0) * falloff;
            if (weight <= 0.0) continue;
            result.amount += weight;
            result.bounce += SceneColorAt(sample_pixel) * weight;
        }
    }
    float normalization = occlusion_gain / float(occlusion_directions * occlusion_steps);
    result.amount = saturate(result.amount * normalization);
    result.bounce *= normalization;
    return result;
}

float3 ApplyOcclusion(float3 color, int2 pixel, float3 position, float3 normal) {
    if (occlusion_strength <= 0.0 && bounce_strength <= 0.0) return color;
    Occlusion occlusion = SampleOcclusion(pixel, position, normal);
    float visibility = 1.0 - occlusion_strength * occlusion.amount;
    return color * visibility + color * occlusion.bounce * bounce_strength;
}
