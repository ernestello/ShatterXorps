// File: light_ray.vert
#version 450

// Input attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

// UBO (set 0, binding 0)
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;

void main() {
    // Multiply the vertex position by the full MVP matrix
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor; // Pass along the vertex color (or you can compute something else)
}
