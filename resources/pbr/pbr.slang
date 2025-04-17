// pbr.slang

struct GlobalUniforms
{
    float time;
    float frame;
};

struct Light
{
    float4 position;
    float4 color;
};

struct CameraUniforms
{
    float4x4 view;
    float4x4 proj;
};

struct ModelUniform
{
    float4x4 model;
    float4x4 modelInvTranspose;
    float4 attributeFlags1;
    float4 attributeFlags2;
};

struct FrameFreq
{
    ConstantBuffer<GlobalUniforms> uGlobal;
    StructuredBuffer<Light> lights;
    ConstantBuffer<CameraUniforms> uCamera;
}
ParameterBlock<FrameFreq> frameFreq;

struct DrawFreq
{
    StructuredBuffer<ModelUniform> uModels;
}
ParameterBlock<DrawFreq> drawFreq;

struct VSInput {
    float4 aPosition : POSITION;
    float4 aNormal   : NORMAL;
    float4 aTangent  : TANGENT;
    float2 aTexCoord : TEXCOORD0;
    // float4 aColor    : COLOR0;    // Or COLOR<n>
    // uint4  aJoints   : BLENDINDICES0; // Or BLENDINDICES<n>
    // float4 aWeights  : BLENDWEIGHT0; // Or BLENDWEIGHT<n>
};

// Vertex Shader Outputs (Varyings + Position) using Semantics
struct VSOutput {
    float4 vPosition : V_WORLD_POSITION;
    float4 vNormal   : V_WORLD_NORMAL;
    // float4 vTangent  : V_TANGENT;      // Example custom semantic
    // float2 vTexCoord : TEXCOORD0;      // Can often re-use input semantics for pass-through
    // float4 vColor    : COLOR0;         // Can often re-use input semantics for pass-through

    // System value semantic for clip space position (required)
    float4 gl_Position : SV_Position;
};

[shader("vertex")]
VSOutput main(uniform uint modelIndex, VSInput input)
{
    VSOutput output;

    // Access uniforms via parameter blocks and push constants
    ModelUniform modelData = drawFreq.uModels[modelIndex];

    float4 worldPosition = mul(modelData.model, input.aPosition);
    float4 worldNormal   = mul(modelData.modelInvTranspose, input.aNormal);

    output.vPosition = worldPosition;
    output.vNormal   = worldNormal;
    output.gl_Position = mul(frameFreq.uCamera.proj, mul(frameFreq.uCamera.view, worldPosition));

    return output;
}

[shader("fragment")]
float4 fs() : SV_Target
{
    return float4(1.0);
}
