// curves.slang

struct CameraData
{
    float4x4 view;
    float4x4 proj;
};

ConstantBuffer<CameraData> camera;

static const float3 positions[6] = float3[](
    float3(0.0, 0.0, 0.0), float3(1.0, 0.0, 0.0), // (along instance's X axis)
    float3(0.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), // (along instance's Y axis)
    float3(0.0, 0.0, 0.0), float3(0.0, 0.0, 1.0)  // (along instance's Z axis)
);

static const float3 colors[6] = float3[](
    float3(1.0, 0.0, 0.0), float3(1.0, 0.0, 0.0), // Red
    float3(0.0, 1.0, 0.0), float3(0.0, 1.0, 0.0), // Green
    float3(0.0, 0.0, 1.0), float3(0.0, 0.0, 1.0)  // Blue
);

struct A2V
{
    float4x4 trans;
    int vertexID : SV_VertexID;
};

struct V2F
{
    float4 position : SV_Position;
    float3 color;
};


[shader("vertex")]
V2F vs(A2V a2v) : SV_Position
{
    V2F v2f;

    v2f.position = mul(camera.proj * camera.view, float4(positions[a2v.vertexID], 1.0));
    v2f.color = colors[a2v.vertexID];

    return v2f;
}

[shader("fragment")]
float4 fs(V2F v2f) : SV_Target
{
    return float4(v2f.color, 1.0);
}
