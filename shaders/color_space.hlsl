float3 SrgbToLinear(float3 color) {
    float3 low = color / 12.92;
    float3 high = pow(max((color + 0.055) / 1.055, 0.0), 2.4);
    return lerp(high, low, step(color, 0.04045));
}

float3 LinearToSrgb(float3 color) {
    float3 low = color * 12.92;
    float3 high = 1.055 * pow(max(color, 0.0), 1.0 / 2.4) - 0.055;
    return lerp(high, low, step(color, 0.0031308));
}
