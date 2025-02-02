///////////////////////////////////////////
// FILE: shadow.vert
///////////////////////////////////////////
#version 450

// For a minimal shadow pass, we just transform each vertex
// using the "lightViewProj" matrix, outputting gl_Position.
// If we don't have a fragment shader, the pipeline does a depth-only pass.

// Suppose you match the same vertex attribute binding
layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform LightUBO {
    mat4 lightViewProj;
} ubo;

void main()
{
    // Transform the vertex into light clip-space
    gl_Position = ubo.lightViewProj * vec4(inPosition, 1.0);
}
