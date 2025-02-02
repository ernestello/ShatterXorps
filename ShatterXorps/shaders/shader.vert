#version 450

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inColor;
layout(location=2) in vec3 inNormal; // new input

layout(location=0) out vec3 fragColor;
layout(location=1) out vec3 fragWorldPos;
layout(location=2) out vec3 fragNorm; // pass to fragment

layout(binding=0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main()
{
    vec4 worldPos    = ubo.model * vec4(inPosition, 1.0);
    gl_Position      = ubo.proj * ubo.view * worldPos;

    fragColor        = inColor;
    fragWorldPos     = worldPos.xyz;

    // transform normal if your model can rotate/scale:
    fragNorm         = mat3(ubo.model) * inNormal;
}
