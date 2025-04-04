#version 450
#pragma shader_stage(vertex)

#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec4 aTangent;
layout(location = 3) in vec2 aTexCoord;
// layout(location = 4) in vec4 aColor;
// layout(location = 5) in uvec4 aJoints;
// layout(location = 6) in vec4 aWeights;

// updated once per frame
layout(set = 0, binding = 0) uniform GlobalUniforms {
    float time;
    float frame;
} uGlobal;

struct Light {
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 1) buffer Lights {
    Light lights[];
};

layout(set = 0, binding = 2) uniform CameraUniforms {
    mat4 view;
    mat4 proj;
} uCamera;

layout(set = 1, binding = 0) uniform ModelUniforms {
    mat4 model;
    mat4 modelInvTranspose;
    vec4 attributeFlags1;
    vec4 attributeFlags2;
} uModel;

// layout(set = 2, binding = 0) uniform MaterialUniforms {
//     vec4 baseColor;
//     float metallic;
//     float roughness;
//     float ambientOcclusion;
//     float emissive;
// } uMaterial;

layout(location = 0) out vec4 vPosition;
layout(location = 1) out vec4 vNormal;
// layout(location = 2) out vec4 vTangent;
// layout(location = 3) out vec2 vTexCoord;
// layout(location = 4) out vec4 vColor;

void main() {
    vPosition = uModel.model * aPosition;
    vNormal = uModel.modelInvTranspose * aNormal;
    gl_Position = uCamera.proj * uCamera.view * vPosition;
}
