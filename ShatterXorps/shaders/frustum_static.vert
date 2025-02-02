#version 450

layout(location=0) in vec3 inPosition;

// We'll store the "cameraViewProj" matrix in set=0, binding=0
layout(set=0, binding=0) uniform CameraBlock {
    mat4 cameraViewProj;
} cam;

void main()
{
    gl_Position = cam.cameraViewProj * vec4(inPosition, 1.0);
}
