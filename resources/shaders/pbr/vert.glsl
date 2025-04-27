#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec4 aTangent;
layout(location = 3) in vec2 aTexCoord;

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

layout(location = 0) out vec4 vPosition;
layout(location = 1) out vec4 vNormal;

void main()
{
    vPosition = model.model * aPosition;
    vNormal = model.model * aNormal;
    gl_Position = camera.proj * camera.view * vPosition;
}
