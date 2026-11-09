struct CameraFrame {
    row_major float4x4 reprojection;
    row_major float4x4 view;
    row_major float4x4 projection;
    float2 jitter;
    uint valid;
    uint padding;
};
