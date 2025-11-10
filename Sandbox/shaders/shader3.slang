struct VertexIn
{
    float2 inPosition;
    float3 inColor;
};

struct VertexOut
{
    float4 position : SV_Position;
    float3 color;
};

struct CameraBufferData
{
    float4x4 view;
    float4x4 projection;
};

struct ObjectBufferData
{
    float4x4 model;
};

[shader("vertex")]
VertexOut vertMain(VertexIn input,
                   ConstantBuffer<CameraBufferData> ubo_camera,
                   ConstantBuffer<ObjectBufferData> ubo_object)
{
    VertexOut output;
    output.position = mul(ubo_camera.projection,
                          mul(ubo_camera.view,
                              mul(ubo_object.model, float4(input.inPosition, 0.0, 1.0))));
    output.color = input.inColor;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOut input) : SV_TARGET
{
    return float4(input.color, 1.0);
}
