// Buffer.h
#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <stdexcept>
#include "PhysicalDevice.h"

class Buffer {
public:
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size; // Add size member

    // **Default constructor**
    Buffer();

    // **Parameterized constructor**
    Buffer(VkDevice device, PhysicalDevice& physicalDevice, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    // **Destructor**
    ~Buffer();

    // **Method to destroy buffer and free memory**
    void destroy(VkDevice device);

private:
    // **Function to create buffer**
    void createBuffer(VkDevice device, PhysicalDevice& physicalDevice, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory);
};

#endif // BUFFER_H
