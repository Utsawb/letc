#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 aPosition;

layout(set = 0, binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
} uCamera;

void main() {
    gl_Position = uCamera.proj * uCamera.view * vec4(aPosition, 1.0);
    gl_PointSize = 10.0;
}