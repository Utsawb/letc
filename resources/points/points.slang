struct Camera
{
    float4x4 view;
    float4x4 proj;
};

cbuffer GlobalUniforms
{
    Camera camera;
};

struct VSOut
{
    float4 sv_position : SV_Position;
    float sv_pointSize : SV_PointSize;
};

[shader("vertex")]
VSOut vs(float3 position : POSITION)
{
    VSOut output;

    output.sv_position = mul(camera.proj * camera.view, float4(position, 1.0));
    output.sv_pointSize = 10.0;

    return output;
}

[shader("fragment")]
float4 fs() : SV_Target
{
    return float4(1.0);
}
