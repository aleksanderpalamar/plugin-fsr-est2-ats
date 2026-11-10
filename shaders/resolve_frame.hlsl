#include "camera_frame.hlsl"
#include "matrix_math.hlsl"

cbuffer GameResolve : register(b0) {
    float4 resolve[16];
};

cbuffer ResolveParameters : register(b1) {
    float2 render_size;
    float max_move;
    float min_turn_cosine;
};

RWStructuredBuffer<CameraFrame> current_frame : register(u0);

static const uint exponent_mask = 0x7F800000u;
static const float affine_tolerance = 1e-6;
static const float unit_tolerance = 1e-3;
static const float max_jitter = 1.0;

bool finite_constants() {
    [unroll] for (int index = 0; index < 12; ++index) {
        if (!all((asuint(resolve[index]) & exponent_mask) != exponent_mask))
            return false;
    }
    return true;
}

float4x4 affine_inverse(float3x3 linear_inverse, float3 offset) {
    float3 moved = -mul(linear_inverse, offset);
    return float4x4(float4(linear_inverse[0], moved.x), float4(linear_inverse[1], moved.y),
        float4(linear_inverse[2], moved.z), float4(0.0, 0.0, 0.0, 1.0));
}

bool small_step(float4x4 relative) {
    float cosine = (relative[0][0] + relative[1][1] + relative[2][2] - 1.0) * 0.5;
    return length(offset_part(relative)) <= max_move && cosine >= min_turn_cosine;
}

[numthreads(1, 1, 1)]
void ResolveMain() {
    float4x4 motion = float4x4(resolve[0], resolve[1], resolve[2], resolve[3]);
    float4x4 relative = float4x4(resolve[4], resolve[5], resolve[6], resolve[7]);
    LinearInverse rotation = inverse3(linear_part(relative));
    float4x4 projection = mul(motion, affine_inverse(rotation.value, offset_part(relative)));
    Inverse projection_inverse = inverse4(projection);
    float4x4 depth_flip = float4x4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 1.0, 0.0, 0.0, 0.0, 1.0);
    CameraFrame frame;
    frame.reprojection = mul(mul(motion, projection_inverse.value), depth_flip);
    frame.view = relative;
    frame.projection = projection;
    frame.jitter = resolve[8].xy * render_size;
    bool usable = finite_constants() && rotation.valid && projection_inverse.valid && affine(relative);
    usable = usable && all(resolve[11].xy == render_size) && all(abs(frame.jitter) <= max_jitter);
    usable = usable && abs(abs(projection[3][2]) - 1.0) < unit_tolerance && small_step(relative);
    frame.valid = usable ? 1 : 0;
    frame.padding = 0;
    current_frame[0] = frame;
}
