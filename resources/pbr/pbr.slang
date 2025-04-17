struct Global
{
    float time;
    float frame;
};

struct Light
{
    float4 position;
    float4 color;
};

struct Camera
{
    float4x4 view;
    float4x4 proj;
};

struct FrameData
{
    Global global;
    StructuredBuffer<Light> Lights;
    Camera camera;
};

ParameterBlock<FrameData> frameData;

struct VSIn
{
    float4 position;
    float4 normal;
    float4 tangent;
    float2 uv;
};

struct VSOut
{
    float4 sv_position : SV_Position;
    float sv_pointSize : SV_PointSize;
};

[shader("vertex")]
VSOut vs(VSIn vsin)
{
    VSOut output;

    output.sv_position = mul(frameData.camera.proj * frameData.camera.view, vsin.position);
    output.sv_pointSize = 10.0;

    return output;
}

[shader("fragment")]
float4 fs() : SV_Target
{
    return float4(1.0);
}
