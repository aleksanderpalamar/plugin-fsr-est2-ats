#include "scene_tracer_common.hlsl"
#include "scene_tracer_bvh.hlsl"

float3 Background(Ray ray) {
    float blend = saturate((normalize(ray.direction).y + 1.0) * 0.5);
    return background_bottom * (1.0 - blend) + background_top * blend;
}

bool Shadowed(Hit hit, float3 to_light) {
    Ray shadow;
    shadow.origin = hit.position + hit.normal * epsilon;
    shadow.direction = to_light;
    Hit blocker;
    return TraverseScene(shadow, epsilon, infinity, true, blocker);
}

float3 DirectLight(Hit hit, SceneMaterial material) {
    float3 result = material.base_color * ambient;
    [loop]
    for (uint index = 0; index < light_count; ++index) {
        SceneLight light = lights[index];
        float cosine = dot(hit.normal, light.to_light);
        if (light.intensity <= 0.0 || cosine <= 0.0) continue;
        if (Shadowed(hit, light.to_light)) continue;
        result += material.base_color * light.color * (light.intensity * cosine);
    }
    return result;
}

float3 Trace(Ray ray) {
    float3 color = 0.0;
    float3 throughput = 1.0;
    [loop]
    for (uint bounce = 0; bounce <= max_bounces; ++bounce) {
        Hit hit;
        if (!TraverseScene(ray, epsilon, infinity, false, hit)) return color + throughput * Background(ray);
        if (hit.material >= material_count) return color;
        SceneMaterial material = materials[hit.material];
        float3 local = DirectLight(hit, material);
        if (material.reflectivity == 0.0 || bounce == max_bounces) return color + throughput * local;
        color += throughput * local * (1.0 - material.reflectivity);
        throughput *= material.reflectivity;
        ray.direction = normalize(reflect(ray.direction, hit.normal));
        ray.origin = hit.position + hit.normal * epsilon;
    }
    return color;
}

Ray CameraRay(uint2 pixel) {
    float2 ndc = float2(2.0 * (float(pixel.x) + 0.5) / float(image_width) - 1.0,
        1.0 - 2.0 * (float(pixel.y) + 0.5) / float(image_height));
    Ray ray;
    ray.origin = camera_position;
    ray.direction = normalize(camera_forward + camera_horizontal * ndc.x + camera_vertical * ndc.y);
    return ray;
}

[numthreads(8, 8, 1)]
void TraceSceneMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= image_width || id.y >= image_height) return;
    output_image[id.xy] = float4(Trace(CameraRay(id.xy)), 1.0);
}
