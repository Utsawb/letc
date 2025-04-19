#version 450
#pragma shader_stage(vertex)

#extension GL_EXT_debug_printf : enable

layout(location = 0) in mat4 trans;

layout(set = 0, binding = 0) uniform CameraUniforms {
    mat4 view;
    mat4 proj;
} uCamera;

layout(location = 0) out vec3 color;

const vec3 positions[6] = vec3[](
    vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), // (along instance's X axis)
    vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), // (along instance's Y axis)
    vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0)  // (along instance's Z axis)
);

const vec3 colors[6] = vec3[](
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), // Red
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), // Green
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0)  // Blue
);

void main()
{
    gl_Position = uCamera.proj * uCamera.view * trans * vec4(positions[gl_VertexIndex] * 0.1, 1.0);
    color = colors[gl_VertexIndex];
}
