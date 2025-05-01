#version 450
#pragma shader_stage(compute)

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(set = 0, binding = 0, rgba8) uniform readonly image2D inputColor;
layout(set = 0, binding = 1) uniform sampler2D inputDepth;
layout(set = 0, binding = 2, rgba8) uniform writeonly image2D outputColor;

const float DEPTH_THRESHOLD = 0.1;
const float BLUR_SIGMA = 6.0;

const int KERNEL_SIZE = 16;
const int KERNEL_RADIUS = KERNEL_SIZE / 2;

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outputColor);

    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoords) + vec2(0.5)) / vec2(imageSize);

    vec2 centerUv = vec2(0.5, 0.5);
    float centerDepth = texture(inputDepth, centerUv).r;

    // if (length(uv - centerUv) < 0.1)
    // {
    //     imageStore(outputColor, pixelCoords, vec4(0.0, 0.0, 0.0, 1.0));
    //     return;
    // }

    float currentDepth = texture(inputDepth, uv).r;
    vec4 inColor = imageLoad(inputColor, pixelCoords);

    float depthDifference = abs(currentDepth - centerDepth);
    vec4 finalColor;

    // --- Apply 8x8 Gaussian Blur ---
    vec4 blurredColor = vec4(0.0);
    float totalWeight = 0.0;
    // Using the hardcoded constant BLUR_SIGMA now
    float twoSigmaSq = 2.0 * BLUR_SIGMA * BLUR_SIGMA; // Precompute for efficiency

    // Iterate through the 8x8 kernel neighborhood
    // Loop ranges adjusted for centering an even-sized kernel
    for (int y = -KERNEL_RADIUS + 1; y <= KERNEL_RADIUS; ++y) {
        for (int x = -KERNEL_RADIUS + 1; x <= KERNEL_RADIUS; ++x) {
            // Calculate coordinates of the neighboring pixel to sample
            ivec2 sampleCoords = pixelCoords + ivec2(x, y);

            // Clamp coordinates to stay within image bounds
            // Important when using imageLoad to prevent out-of-bounds access
            sampleCoords = clamp(sampleCoords, ivec2(0), imageSize - 1);

            // Calculate Gaussian weight for this neighbor
            // Weight decreases exponentially with distance from the center pixel (x=0, y=0)
            float distSq = float(x * x + y * y);
            float weight = exp(-distSq / twoSigmaSq);

            // Load neighbor's color using imageLoad (required for image2D)
            vec4 sampleColor = imageLoad(inputColor, sampleCoords);

            // Accumulate weighted color and total weight
            blurredColor += sampleColor * weight;
            totalWeight += weight;
        }
    }

    // Normalize the blurred color by dividing by the total weight
    // Avoid division by zero if totalWeight is somehow zero
    if (totalWeight > 0.0) {
        finalColor = blurredColor / totalWeight;
    } else {
        finalColor = inColor; // Fallback to original color
    }

    // Write the final color (either original or blurred) to the output image
    imageStore(outputColor, pixelCoords, finalColor);
}
