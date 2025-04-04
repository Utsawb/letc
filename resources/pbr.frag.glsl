#version 450
#pragma shader_stage(fragment)

// #extension GL_EXT_debug_printf : enable

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
// layout(location = 2) in vec4 vTangent;
// layout(location = 3) in vec2 vTexCoord;
// layout(location = 4) in vec4 vColor;

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
layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);

    for (int i = 0; i < lights.length(); i++) {
        vec3 lightDir = normalize(lights[i].position.xyz - vPosition.xyz);
        float NdotL = max(dot(vNormal.xyz, lightDir), 0.0);
        fragColor.rgb += NdotL * lights[i].color.rgb;
    }
}
