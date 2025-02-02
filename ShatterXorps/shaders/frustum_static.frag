#version 450

layout(location=0) out vec4 outColor;

void main()
{
    // Semi‐transparent green lines
    outColor = vec4(0.0, 1.0, 0.0, 0.35);
}
