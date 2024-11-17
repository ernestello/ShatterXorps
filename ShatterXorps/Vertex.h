// Vertex.h
#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

// Initialization: Define Vertex struct | Vertex.h | Used by GraphicsPipeline.cpp and main.cpp | Represents a vertex with position and color | Struct Definition - To define vertex attributes | Depends on rendering requirements | Minimal computing power | Defined once at [line 4 - Vertex.h - global scope] | GPU
struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

    // Binding description
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    // Attribute descriptions
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        // Position attribute
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        // Color attribute
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

#endif // VERTEX_H
