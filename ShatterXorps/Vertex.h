#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

struct Vertex {
    // We now store position, color, AND normal
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal; // NEW

    // Binding description
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    // Attribute descriptions
    // We now have 3 attributes:
    //   location=0 => position
    //   location=1 => color
    //   location=2 => normal
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> ads{};

        // Position
        ads[0].binding = 0;
        ads[0].location = 0;
        ads[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        ads[0].offset = offsetof(Vertex, position);

        // Color
        ads[1].binding = 0;
        ads[1].location = 1;
        ads[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        ads[1].offset = offsetof(Vertex, color);

        // Normal
        ads[2].binding = 0;
        ads[2].location = 2;
        ads[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        ads[2].offset = offsetof(Vertex, normal);

        return ads;
    }
};

#endif // VERTEX_H
