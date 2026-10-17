struct SceneNode {
    float3 bounds_min;
    uint first_primitive;
    float3 bounds_max;
    uint primitive_count;
    uint left_child;
    uint right_child;
    uint2 padding;
};

struct SceneTriangle {
    float3 a;
    uint material;
    float3 b;
    uint has_normals;
    float3 c;
    uint padding_c;
    float3 normal_a;
    uint padding_normal_a;
    float3 normal_b;
    uint padding_normal_b;
    float3 normal_c;
    uint padding_normal_c;
};

struct SceneMaterial {
    float3 base_color;
    float reflectivity;
};

struct SceneLight {
    float3 to_light;
    float intensity;
    float3 color;
    float padding;
};

StructuredBuffer<SceneNode> nodes : register(t0);
StructuredBuffer<SceneTriangle> triangles : register(t1);
StructuredBuffer<SceneMaterial> materials : register(t2);
StructuredBuffer<SceneLight> lights : register(t3);
RWTexture2D<float4> output_image : register(u0);

cbuffer TraceParameters : register(b0) {
    float3 camera_position;
    uint image_width;
    float3 camera_forward;
    uint image_height;
    float3 camera_horizontal;
    uint node_count;
    float3 camera_vertical;
    uint light_count;
    float3 ambient;
    float epsilon;
    float3 background_bottom;
    uint max_bounces;
    float3 background_top;
    uint material_count;
};

struct Ray {
    float3 origin;
    float3 direction;
};

struct Hit {
    float distance;
    float3 position;
    float3 normal;
    uint material;
};

static const float infinity = 1.0e30;
static const float determinant_epsilon = 1.0e-8;
static const uint stack_size = 40;
