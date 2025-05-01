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
    // 1. Define Directional Light Properties
    //    - Direction: Where the light is *coming from*. Normalize it.
    //      (e.g., low on the horizon for sunset, slightly positive X/Z, positive Y)
    //    - Color: A warm orange/red typical of sunsets.
    vec3 lightDirection = normalize(vec3(0.8, 0.3, 0.2)); // Adjust X/Y/Z for desired sunset angle
    vec3 lightColor = vec3(1.0, 0.55, 0.2); // Sunset orange-red color (Intensity included here)

    // Optional: Add a dim ambient light so shadowed areas aren't pitch black
    vec3 ambientColor = vec3(0.15, 0.08, 0.05); // Dim ambient sunset color

    // 2. Get the Surface Normal
    //    - Assuming vNormal is already in world space.
    //    - Normalize it to ensure it's a unit vector.
    vec3 N = normalize(vNormal.xyz);

    // 3. Calculate Diffuse Lighting (Lambertian)
    //    - Computes how much the surface normal aligns with the light direction.
    //    - dot(N, lightDirection) gives cosine of the angle between them.
    //    - max(..., 0.0) ensures surfaces facing away from the light aren't negatively lit.
    float diffuseFactor = max(dot(N, lightDirection), 0.0);

    // 4. Calculate Final Color
    //    - Multiply the light color by the diffuse factor.
    //    - Add the ambient color.
    vec3 diffuseContribution = lightColor * diffuseFactor;
    vec3 finalColor = ambientColor + diffuseContribution;

    // 5. Output the color
    //    - Use the calculated RGB color.
    //    - Set alpha to 1.0 for opaque surfaces.
    color = vec4(finalColor, 1.0);
}
