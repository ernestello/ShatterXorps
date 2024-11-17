// UniformBufferObject.h
#ifndef UNIFORM_BUFFER_OBJECT_H
#define UNIFORM_BUFFER_OBJECT_H

#include <glm/glm.hpp>

// Initialization: Define UniformBufferObject struct | UniformBufferObject.h | Used by main.cpp and other classes | Holds transformation matrices for shaders | Struct Definition - To pass data to shaders | Depends on shader uniform requirements | Minimal computing power | Defined once at [line 4 - UniformBufferObject.h - global scope] | GPU
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

#endif // UNIFORM_BUFFER_OBJECT_H
