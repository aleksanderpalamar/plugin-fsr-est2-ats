#include "camera_frame.hlsl"

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

static const float singular = 1e-12;

struct Inverse {
    float4x4 value;
    bool valid;
};

Inverse inverse4(float4x4 m) {
    float b00 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    float b01 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
    float b02 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
    float b03 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    float b04 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
    float b05 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
    float b06 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
    float b07 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
    float b08 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
    float b09 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
    float b10 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
    float b11 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
    float determinant = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
    float4x4 r;
    r[0] = float4(m[1][1] * b11 - m[1][2] * b10 + m[1][3] * b09, m[0][2] * b10 - m[0][1] * b11 - m[0][3] * b09,
        m[3][1] * b05 - m[3][2] * b04 + m[3][3] * b03, m[2][2] * b04 - m[2][1] * b05 - m[2][3] * b03);
    r[1] = float4(m[1][2] * b08 - m[1][0] * b11 - m[1][3] * b07, m[0][0] * b11 - m[0][2] * b08 + m[0][3] * b07,
        m[3][2] * b02 - m[3][0] * b05 - m[3][3] * b01, m[2][0] * b05 - m[2][2] * b02 + m[2][3] * b01);
    r[2] = float4(m[1][0] * b10 - m[1][1] * b08 + m[1][3] * b06, m[0][1] * b08 - m[0][0] * b10 - m[0][3] * b06,
        m[3][0] * b04 - m[3][1] * b02 + m[3][3] * b00, m[2][1] * b02 - m[2][0] * b04 - m[2][3] * b00);
    r[3] = float4(m[1][1] * b07 - m[1][0] * b09 - m[1][2] * b06, m[0][0] * b09 - m[0][1] * b07 + m[0][2] * b06,
        m[3][1] * b01 - m[3][0] * b03 - m[3][2] * b00, m[2][0] * b03 - m[2][1] * b01 + m[2][2] * b00);
    Inverse result;
    result.valid = abs(determinant) > singular;
    result.value = result.valid ? r / determinant : r;
    return result;
}

struct LinearInverse {
    float3x3 value;
    bool valid;
};

float3x3 linear_part(float4x4 m) {
    return float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
}

float3 offset_part(float4x4 m) {
    return float3(m[0][3], m[1][3], m[2][3]);
}

LinearInverse inverse3(float3x3 m) {
    float3 c0 = cross(m[1], m[2]);
    float3 c1 = cross(m[2], m[0]);
    float3 c2 = cross(m[0], m[1]);
    float determinant = dot(m[0], c0);
    LinearInverse result;
    result.valid = abs(determinant) > singular;
    result.value = transpose(float3x3(c0, c1, c2)) / (result.valid ? determinant : 1.0);
    return result;
}

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

bool affine(float4x4 view) {
    return all(abs(view[3] - float4(0.0, 0.0, 0.0, 1.0)) < 1e-6);
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
