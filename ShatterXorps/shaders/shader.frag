////////////////////////////////////////////////////////////
// FILE: shader.frag (UPDATED)
////////////////////////////////////////////////////////////

#version 450

// Inputs from the vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragNorm;

// Output to the framebuffer
layout(location = 0) out vec4 outColor;

// ----------------------------
// UBO for camera & light
//   set=0, binding=1 => LightUBO
// ----------------------------
layout(set = 0, binding = 1) uniform LightUBO {
    mat4 lightViewProj;
    vec4 lightPos; // world-space position if needed
} lightData;

// ----------------------------
// set=1, binding=1 => shadow map sampler
// (We've updated the binding to '1')
// ----------------------------
layout(set = 1, binding = 1) uniform sampler2D shadowMap;

// Some small bias used in the depth comparison:
const float bias = 0.001; // tweak as needed

void main()
{
    ///////////////////////////////////////////
    // 1) Basic shading (Lambert) in world space
    ///////////////////////////////////////////
    vec3 baseColor = fragColor;
    vec3 normal    = normalize(fragNorm);

    // Light direction for simple N·L shading:
    vec3 lightDir  = normalize(lightData.lightPos.xyz - fragWorldPos);
    float ndotl    = max(dot(normal, lightDir), 0.0);

    ///////////////////////////////////////////
    // 2) Compute "shadowUV" and "currentDepth"
    //    in [0..1] from the light's clip space
    ///////////////////////////////////////////
    // Project fragment into the light's clip coords:
    vec4 lightClip = lightData.lightViewProj * vec4(fragWorldPos, 1.0);

    // Convert to normalized device coords:
    vec3 ndc = lightClip.xyz / lightClip.w; // [-1..1] in X,Y,Z

    // The shadow sampler uses X,Y in [0..1]:
    vec2 shadowUV = ndc.xy * 0.5 + 0.5;

    // The hardware depth is in [0..1] => do the same:
    float currentDepth = ndc.z * 0.5 + 0.5;

    ///////////////////////////////////////////
    // 3) Check if we're outside [0..1]
    //    If so, skip the shadow test
    ///////////////////////////////////////////
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0) 
    {
        // Outside the shadow map => no shadow
        float ambient = 0.1;
        outColor = vec4(baseColor * (ndotl + ambient), 1.0);
        return;
    }

    ///////////////////////////////////////////
    // 4) Manual PCF (3×3) in the shadow map
    ///////////////////////////////////////////
    float shadowSum = 0.0;
    int samples     = 0;

    // Example 3×3 PCF with a 2048×2048 shadow
    float pcfScale = 1.0 / 2048.0;

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            // small offset
            vec2 offset = vec2(x, y) * pcfScale;
            float storedDepth = texture(shadowMap, shadowUV + offset).r;

            // Compare
            float litSample = (currentDepth <= storedDepth + bias) ? 1.0 : 0.0;
            shadowSum += litSample;
            samples++;
        }
    }

    float shadowFactor = shadowSum / float(samples);

    ///////////////////////////////////////////
    // 5) Final color = (Lambert * shadowFactor) + ambient
    ///////////////////////////////////////////
    float ambient      = 0.1;
    float diffuse      = ndotl * shadowFactor;
    vec3 finalColor    = baseColor * (diffuse + ambient);

    outColor = vec4(finalColor, 1.0);
}
