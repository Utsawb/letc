// pbr.slang

struct S_Global
{
    float time;
    float frame;
};

struct S_Light
{
    float4 position;
    float4 color;
};

struct S_Camera
{
    float4x4 view;
    float4x4 proj;
};

struct S_Model
{
    float4x4 model;
    float4x4 modelInvTranspose;
    float4 attributeFlags1;
    float4 attributeFlags2;
};

struct Frame
{
    ConstantBuffer<S_Global> globals;
    StructuredBuffer<S_Light> lights;
};
ParameterBlock<Frame> frame;

struct Draw
{
    StructuredBuffer<S_Model> models;
};
ParameterBlock<Draw> draw;

struct View
{
    ConstantBuffer<S_Camera> cameras;
};
ParameterBlock<View> view;

struct A2V
{
    float4 aPosition;
    float4 aNormal;
    float4 aTangent;
    float2 aTexCoord;
};

struct V2F
{
    float4 vPosition;
    float4 vNormal;
    float4 vTangent;
    float2 vTexCoord;

    float4 cPosition : SV_Position;
};

[shader("vertex")]
V2F vs(uniform uint modelIdx, A2V a2v)
{
    V2F v2f;
    var model = draw.models[modelIdx];

    var funnyLightShit = frame.lights[0].position.x;

    float4 wPosition = mul(model.model, a2v.aPosition);
    float4 wNormal   = mul(model.modelInvTranspose, a2v.aNormal);

    // random data to make sure slang does not optimize away unused
    v2f.vTexCoord.x = frame.globals.time + frame.lights[0].position.x + a2v.aTangent.x + a2v.aTexCoord.x;

    v2f.vPosition = wPosition;
    v2f.vNormal   = wNormal;
    v2f.cPosition = mul(view.cameras.proj * view.cameras.view, wPosition);

    return v2f;
}

[shader("fragment")]
float4 fs(V2F v2f) : SV_Target
{
    float4 color = float4(0.0, 0.0, 0.0, 1.0);
    float3 norm = normalize(v2f.vNormal.xyz);

    for (int i = 0; i < frame.lights.getCount(); i++)
    {
        float3 lightDir = normalize(frame.lights[i].position.xyz - v2f.vPosition.xyz);
        float nDotL = max(dot(norm, lightDir), 0.0);
        color.xyz += nDotL * frame.lights[i].color.xyz;
    }

    return color;
}
