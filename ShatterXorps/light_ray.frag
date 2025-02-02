#version 450

// Color passed from the vertex shader
layout(location = 0) in vec3 fragColor;

// Final color output
layout(location = 0) out vec4 outColor;

void main()
{
    // Simply output the passed color with full alpha=1
    outColor = vec4(fragColor, 1.0);
}
