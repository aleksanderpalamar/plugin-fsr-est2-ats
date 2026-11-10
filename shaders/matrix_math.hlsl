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

bool affine(float4x4 view) {
    return all(abs(view[3] - float4(0.0, 0.0, 0.0, 1.0)) < 1e-6);
}
