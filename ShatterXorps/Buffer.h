// Buffer.h
#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan.h>
#include "PhysicalDevice.h"

// Initialization: Define Buffer class | Buffer.h | Used by main.cpp and other classes | Encapsulates Vulkan buffer functionality | Class Definition - To manage buffer resources | Depends on Vulkan device and PhysicalDevice | Minimal computing power | Defined once at [line 4 - Buffer.h - global scope] | GPU
class Buffer {
public:
    Buffer(VkDevice device, PhysicalDevice& physicalDevice, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~Buffer();

    VkBuffer getBuffer() const { return buffer; }
    VkDeviceMemory getMemory() const { return memory; }
    VkDeviceSize getSize() const { return size; }

    void destroy();

private:
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;

    void createBuffer(PhysicalDevice& physicalDevice, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
};

#endif // BUFFER_H
