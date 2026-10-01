#include "photoreal_common.hlsl"
#include "photoreal_color.hlsl"
#include "photoreal_neural.hlsl"
#include "photoreal_clarity.hlsl"
#include "photoreal_tonemap.hlsl"
#include "photoreal_lut.hlsl"
#include "rcas.hlsl"
#include "photoreal_grain.hlsl"

float4 PhotorealMain(PixelInput input) : SV_Target {
    uint width;
    uint height;
    source_texture.GetDimensions(width, height);
    int2 position = int2(input.position.xy);
    int2 limit = int2(width - 1, height - 1);
    float4 original = source_texture.Load(int3(position, 0));
    float3 color = decode_input != 0 ? SrgbToLinear(original.rgb) : original.rgb;
    color = ApplyNeural(color, input.uv);
    color = ApplyClarity(color, input.uv, position, limit, 1.0 / float2(width, height));
    color = ApplyExposure(color);
    color = ApplyBlackLevel(color);
    color = ShapeHighlights(color);
    color = ApplyContrast(color);
    color = ApplyFilmicTonemap(color);
    color = ApplySplitToning(color);
    color = CompressSaturation(color);
    color = ApplyLut(color);
    return float4(saturate(LinearToSrgb(color)), original.a);
}
