#version 450
#pragma shader_stage(compute)

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform sampler2D colorAttachment;
layout(set = 0, binding = 1) uniform sampler2D depthAttachment;
layout(set = 0, binding = 2, rgba8) uniform writeonly image2D swapchainAttachment;

layout(set = 1, binding = 0) uniform UFrameData {
    float time;
    float frame;
} frameData;

struct SLight {
    vec4 position;
    vec4 color;
};

layout(set = 1, binding = 1) uniform UCamera {
    mat4 proj;
    mat4 view;
} camera;

layout(set = 1, binding = 2) buffer BLights {
    SLight lights[];
};

const float gk[15][15] = float[15][15](
        float[15](0.00005341, 0.00011457, 0.00022273, 0.00038864, 0.00060837, 0.00085660, 0.00107960, 0.00122143, 0.00107960, 0.00085660, 0.00060837, 0.00038864, 0.00022273, 0.00011457, 0.00005341),
        float[15](0.00011457, 0.00024574, 0.00047780, 0.00083340, 0.00130483, 0.00183719, 0.00231557, 0.00261963, 0.00231557, 0.00183719, 0.00130483, 0.00083340, 0.00047780, 0.00024574, 0.00011457),
        float[15](0.00022273, 0.00047780, 0.00092910, 0.00162061, 0.00253669, 0.00357169, 0.00450106, 0.00509276, 0.00450106, 0.00357169, 0.00253669, 0.00162061, 0.00092910, 0.00047780, 0.00022273),
        float[15](0.00038864, 0.00083340, 0.00162061, 0.00282710, 0.00442545, 0.00623084, 0.00785207, 0.00888213, 0.00785207, 0.00623084, 0.00442545, 0.00282710, 0.00162061, 0.00083340, 0.00038864),
        float[15](0.00060837, 0.00130483, 0.00253669, 0.00442545, 0.00692687, 0.00975282, 0.01228765, 0.01389902, 0.01228765, 0.00975282, 0.00692687, 0.00442545, 0.00253669, 0.00130483, 0.00060837),
        float[15](0.00085660, 0.00183719, 0.00357169, 0.00623084, 0.00975282, 0.01373170, 0.01730015, 0.01957541, 0.01730015, 0.01373170, 0.00975282, 0.00623084, 0.00357169, 0.00183719, 0.00085660),
        float[15](0.00107960, 0.00231557, 0.00450106, 0.00785207, 0.01228765, 0.01730015, 0.02179607, 0.02465771, 0.02179607, 0.01730015, 0.01228765, 0.00785207, 0.00450106, 0.00231557, 0.00107960),
        float[15](0.00122143, 0.00261963, 0.00509276, 0.00888213, 0.01389902, 0.01957541, 0.02465771, 0.02789510, 0.02465771, 0.01957541, 0.01389902, 0.00888213, 0.00509276, 0.00261963, 0.00122143),
        float[15](0.00107960, 0.00231557, 0.00450106, 0.00785207, 0.01228765, 0.01730015, 0.02179607, 0.02465771, 0.02179607, 0.01730015, 0.01228765, 0.00785207, 0.00450106, 0.00231557, 0.00107960),
        float[15](0.00085660, 0.00183719, 0.00357169, 0.00623084, 0.00975282, 0.01373170, 0.01730015, 0.01957541, 0.01730015, 0.01373170, 0.00975282, 0.00623084, 0.00357169, 0.00183719, 0.00085660),
        float[15](0.00060837, 0.00130483, 0.00253669, 0.00442545, 0.00692687, 0.00975282, 0.01228765, 0.01389902, 0.01228765, 0.00975282, 0.00692687, 0.00442545, 0.00253669, 0.00130483, 0.00060837),
        float[15](0.00038864, 0.00083340, 0.00162061, 0.00282710, 0.00442545, 0.00623084, 0.00785207, 0.00888213, 0.00785207, 0.00623084, 0.00442545, 0.00282710, 0.00162061, 0.00083340, 0.00038864),
        float[15](0.00022273, 0.00047780, 0.00092910, 0.00162061, 0.00253669, 0.00357169, 0.00450106, 0.00509276, 0.00450106, 0.00357169, 0.00253669, 0.00162061, 0.00092910, 0.00047780, 0.00022273),
        float[15](0.00011457, 0.00024574, 0.00047780, 0.00083340, 0.00130483, 0.00183719, 0.00231557, 0.00261963, 0.00231557, 0.00183719, 0.00130483, 0.00083340, 0.00047780, 0.00024574, 0.00011457),
        float[15](0.00005341, 0.00011457, 0.00022273, 0.00038864, 0.00060837, 0.00085660, 0.00107960, 0.00122143, 0.00107960, 0.00085660, 0.00060837, 0.00038864, 0.00022273, 0.00011457, 0.00005341)
    );

const float baseMinFocusFactor = 0.01;
const float baseMaxFocusFactor = 0.10;
const float absoluteMinDiffThreshold = 0.02;
const float absoluteMaxDiffThreshold = 0.05;
const float minFocusRangeWidth = 0.01;
const float zNear = 0.1;
const float zFar = 1000.0;

float linearize_depth(float d, float near, float far)
{
    return near * far / (far + d * (near - far));
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(swapchainAttachment);

    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y) {
        return;
    }

    vec2 uv = vec2(pixelCoords) / vec2(imageSize);
    vec2 texelSize = 1.0 / vec2(imageSize);
    if (uv == vec2(0.5))
    {
        imageStore(swapchainAttachment, pixelCoords, vec4(0.0));
        return;
    }

    float rawDepth = texture(depthAttachment, uv).r;
    float depthValue = (rawDepth >= 1.0) ? zFar * 10.0 : linearize_depth(rawDepth, zNear, zFar);

    float centreRawDepth = texture(depthAttachment, vec2(0.5)).r;
    float centreDepth = (centreRawDepth >= 1.0) ? zFar * 10.0 : linearize_depth(centreRawDepth, zNear, zFar);

    float depthDiff = abs(depthValue - centreDepth);

    float scaledMinDiff = centreDepth * baseMinFocusFactor;
    float scaledMaxDiff = centreDepth * baseMaxFocusFactor;

    float dynamicMinFocusDiff = max(scaledMinDiff, absoluteMinDiffThreshold);
    float dynamicMaxFocusDiff = max(scaledMaxDiff, absoluteMaxDiffThreshold);

    dynamicMaxFocusDiff = max(dynamicMaxFocusDiff, dynamicMinFocusDiff + minFocusRangeWidth);

    vec4 sharpColor = texture(colorAttachment, uv);
    vec4 blurredColor = vec4(0.0);
    for (int x = -7; x <= 7; ++x)
    {
        for (int y = -7; y <= 7; ++y)
        {
            vec2 sampleUV = clamp(uv + vec2(x, y) * texelSize, vec2(0.0), vec2(1.0));
            blurredColor += texture(colorAttachment, sampleUV) * gk[x + 7][y + 7];
        }
    }
    blurredColor.w = 1.0;
    float blurAmount = smoothstep(dynamicMinFocusDiff, dynamicMaxFocusDiff, depthDiff);
    vec4 finalColor = mix(sharpColor * 1.15, blurredColor, blurAmount);

    imageStore(swapchainAttachment, pixelCoords, finalColor);
}
