static const uint reflection_steps = 24;
static const uint refinement_steps = 4;
static const float reflectance_at_normal = 0.04;
static const float minimum_march_pixels = 2.0;

struct Reflection {
    float3 color;
    float weight;
};

struct ScreenRay {
    float2 start;
    float2 delta;
    float start_inverse_depth;
    float delta_inverse_depth;
};

Reflection NoReflection() {
    Reflection result;
    result.color = 0.0;
    result.weight = 0.0;
    return result;
}

float Fresnel(float3 normal, float3 view_direction) {
    float facing = saturate(dot(normal, -view_direction));
    return reflectance_at_normal + (1.0 - reflectance_at_normal) * pow(1.0 - facing, 5.0);
}

float RayLength(float3 origin, float3 direction) {
    if (direction.z >= 0.0) return max_distance;
    return min(max_distance, (origin.z - near_plane) * 0.99 / -direction.z);
}

ScreenRay ProjectRay(float3 origin, float3 direction) {
    float3 end = origin + direction * RayLength(origin, direction);
    ScreenRay ray;
    ray.start = ProjectUv(origin);
    ray.delta = ProjectUv(end) - ray.start;
    ray.start_inverse_depth = 1.0 / origin.z;
    ray.delta_inverse_depth = 1.0 / end.z - ray.start_inverse_depth;
    return ray;
}

float2 RayUv(ScreenRay ray, float t) {
    return ray.start + ray.delta * t;
}

float RayDepth(ScreenRay ray, float t) {
    return 1.0 / (ray.start_inverse_depth + ray.delta_inverse_depth * t);
}

float AxisExit(float start, float delta) {
    if (delta > 0.0) return (1.0 - start) / delta;
    if (delta < 0.0) return -start / delta;
    return 1.0;
}

float ScreenExit(ScreenRay ray) {
    return saturate(min(AxisExit(ray.start.x, ray.delta.x), AxisExit(ray.start.y, ray.delta.y)));
}

float RefineHit(ScreenRay ray, float before, float after) {
    [unroll]
    for (uint index = 0; index < refinement_steps; ++index) {
        float middle = 0.5 * (before + after);
        float device_depth = DeviceDepthAt(UvPixel(RayUv(ray, middle)));
        bool crossed = !IsBackground(device_depth) && LinearDepth(device_depth) <= RayDepth(ray, middle);
        before = crossed ? before : middle;
        after = crossed ? middle : after;
    }
    return after;
}

Reflection ReflectionHit(ScreenRay ray, float t, float weight) {
    float2 uv = RayUv(ray, t);
    float2 border = min(uv, 1.0 - uv);
    Reflection result;
    result.color = SceneColorAt(UvPixel(uv));
    result.weight = weight * saturate(min(border.x, border.y) * 10.0) * saturate(1.0 - t * t);
    return result;
}

Reflection MarchReflection(ScreenRay ray, float weight, float noise, bool rising) {
    float exit = ScreenExit(ray);
    float span = length(ray.delta * size) * exit;
    if (span < minimum_march_pixels) return NoReflection();
    float start = minimum_march_pixels / span * exit;
    float previous_t = start;
    float previous_depth = RayDepth(ray, start);
    [loop]
    for (uint step_index = 0; step_index < reflection_steps; ++step_index) {
        float t = start + (exit - start) * (float(step_index) + noise) / float(reflection_steps);
        float ray_depth = RayDepth(ray, t);
        float device_depth = DeviceDepthAt(UvPixel(RayUv(ray, t)));
        if (IsBackground(device_depth)) {
            if (rising) return ReflectionHit(ray, t, weight);
        } else {
            float scene = LinearDepth(device_depth);
            if (scene >= min(previous_depth, ray_depth) - thickness && scene <= max(previous_depth, ray_depth))
                return ReflectionHit(ray, RefineHit(ray, previous_t, t), weight);
        }
        previous_t = t;
        previous_depth = ray_depth;
    }
    return NoReflection();
}

Reflection TraceReflection(int2 pixel, float3 position, float3 normal) {
    float3 view_direction = normalize(position);
    float3 direction = reflect(view_direction, normal);
    float facing_fade = saturate(1.0 + direction.z * 4.0);
    float surface_mask = smoothstep(0.6, 0.9, normal.y);
    float weight = Fresnel(normal, view_direction) * facing_fade * surface_mask;
    if (weight <= 0.0) return NoReflection();
    return MarchReflection(ProjectRay(position, direction), weight,
        InterleavedGradientNoise(float2(pixel)), direction.y > 0.0);
}

float3 ApplyReflection(float3 color, int2 pixel, float3 position, float3 normal) {
    if (reflection_strength <= 0.0) return color;
    Reflection reflection = TraceReflection(pixel, position, normal);
    return lerp(color, reflection.color, saturate(reflection_strength * reflection.weight));
}
