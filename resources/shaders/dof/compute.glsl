#version 450
#pragma shader_stage(compute)

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0, rgba8) uniform readonly image2D inputColor;
layout(set = 0, binding = 1, r32f) uniform readonly image2D inputDepth;
layout(set = 0, binding = 2, rgba8) uniform writeonly image2D outputColor;

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);

    ivec2 imageSize = imageSize(outputColor);
    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y) {
        return;
    }

    vec4 inColor = imageLoad(inputColor, pixelCoords);
    float depthValue = imageLoad(inputDepth, pixelCoords).r;

    vec4 processedColor = inColor;
    processedColor = vec4(1.0, 1.0, 1.0, 1.0);

    imageStore(outputColor, pixelCoords, processedColor);
}
