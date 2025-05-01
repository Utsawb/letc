#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;

layout(set = 0, binding = 0) uniform UFrameData {
    float time;
    float frame;
} frameData;

struct SLight {
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 1) uniform UCamera {
    mat4 proj;
    mat4 view;
} camera;

layout(set = 0, binding = 2) buffer BLights {
    SLight lights[];
};

layout(set = 1, binding = 0) uniform UModel {
    mat4 model;
    mat4 model_inv_t;
} model;

layout(location = 0) out vec4 color;

void main()
{
    vec3 worldPos = vPosition.xyz;
    vec3 worldNor = normalize(vNormal.xyz);
    vec3 diffuseColor = vec3(0.0);
    int numLights = lights.length();

    for (int i = 0; i < numLights; ++i)
    {
        vec3 lightPos = lights[i].position.xyz;
        vec3 lightCol = lights[i].color.rgb;
        vec3 lightDir = normalize(lightPos - worldPos.xyz);
        float diffuseFactor = max(dot(worldNor, lightDir), 0.0);
        diffuseColor += lightCol * diffuseFactor * exp(-0.1 * pow(length(lightPos - worldPos.xyz), 2));
    }
    vec3 finalColor = vec3(0.05) + diffuseColor;
    color = vec4(clamp(finalColor, 0.0, 1.0), 1.0);

    // vec3 normed = normalize(vNormal.xyz);
    // color = vec4(normed, 1.0);
}
