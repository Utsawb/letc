// curves.slang

struct CameraData
{
    float4x4 view;
    float4x4 proj;
};

ConstantBuffer<CameraData> camera;

[shader("vertex")]
float4 vs(float3 position : POSITION) : SV_Position
{
    float4x4 viewProj = mul(camera.proj, camera.view);
    return mul(viewProj, float4(position, 1.0));
}

[shader("fragment")]
float4 fs() : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}
