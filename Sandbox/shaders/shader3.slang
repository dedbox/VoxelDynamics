struct CameraBufferData
{
    float4x4 view;
    float4x4 projection;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraBufferData> gCam : register(b0, space0);

struct ObjectBufferData
{
    float4x4 model;
};

[[vk::binding(0, 2)]]
ConstantBuffer<ObjectBufferData> gObj : register(b0, space2);

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

[shader("vertex")]
VertexOut vertMain(VertexIn input)
{
    VertexOut output;
    output.position = mul(gCam.projection, mul(gCam.view, mul(gObj.model, float4(input.inPosition, 0.0, 1.0))));
    output.color = input.inColor;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOut input) : SV_TARGET
{
    return float4(input.color, 1.0);
}
