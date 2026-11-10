#include "camera_frame.hlsl"
#include "matrix_math.hlsl"

cbuffer GameCamera : register(b0) {
    row_major float4x4 game_view;
    row_major float4x4 game_view_projection;
};

cbuffer ReprojectionParameters : register(b1) {
    float2 render_size;
    uint history_valid;
    uint parameters_padding;
};

StructuredBuffer<CameraFrame> previous_frame : register(t0);
RWStructuredBuffer<CameraFrame> current_frame : register(u0);

float4x4 projection_of(float4x4 view_projection, float3x3 linear_inverse, float3 offset) {
    float4x4 projection;
    [unroll] for (int row = 0; row < 4; ++row) {
        float3 columns = mul(view_projection[row].xyz, linear_inverse);
        projection[row] = float4(columns, view_projection[row].w - dot(columns, offset));
    }
    return projection;
}

float4x4 relative_view(float4x4 previous, float4x4 current, float3x3 current_inverse) {
    float3x3 rotation = mul(linear_part(previous), current_inverse);
    float3 current_offset = offset_part(current);
    float3x3 change = rotation - float3x3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
    float3 offset = (offset_part(previous) - current_offset) - mul(change, current_offset);
    return float4x4(float4(rotation[0], offset.x), float4(rotation[1], offset.y),
        float4(rotation[2], offset.z), float4(0.0, 0.0, 0.0, 1.0));
}

[numthreads(1, 1, 1)]
void ReprojectMain() {
    LinearInverse view_inverse = inverse3(linear_part(game_view));
    float4x4 projection = projection_of(game_view_projection, view_inverse.value, offset_part(game_view));
    float w = projection[3][2];
    CameraFrame frame;
    frame.jitter = float2(projection[0][2] / w * render_size.x * 0.5, -projection[1][2] / w * render_size.y * 0.5);
    projection[0][2] = 0.0;
    projection[1][2] = 0.0;
    Inverse projection_inverse = inverse4(projection);
    frame.view = game_view;
    frame.projection = projection;
    float4x4 previous_view = game_view;
    float4x4 previous_projection = projection;
    if (history_valid != 0) {
        previous_view = previous_frame[0].view;
        previous_projection = previous_frame[0].projection;
    }
    float4x4 relative = relative_view(previous_view, game_view, view_inverse.value);
    frame.reprojection = mul(previous_projection, mul(relative, projection_inverse.value));
    bool usable = view_inverse.valid && projection_inverse.valid && affine(game_view);
    frame.valid = usable && abs(abs(w) - 1.0) < 1e-3 ? 1 : 0;
    frame.padding = 0;
    current_frame[0] = frame;
}
