// Buffer.cpp

#include "Buffer.h"
#include <stdexcept>

// Initialization: Define Buffer constructor | Buffer.cpp | Used by main.cpp - line where Buffer is instantiated | Initializes buffer with given parameters | Constructor - To allocate and bind buffer memory | Depends on PhysicalDevice and Vulkan device | Minimal computing power | Once per buffer creation at [line 9 - Buffer.cpp - constructor] | GPU
Buffer::Buffer(VkDevice device, PhysicalDevice& physicalDevice, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : device(device), buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), size(size) {
    createBuffer(physicalDevice, size, usage, properties);
}

// Initialization: Define Buffer destructor | Buffer.cpp | Used by main.cpp - line where Buffer is destroyed | Destroys buffer and frees memory | Destructor - To clean up resources | Depends on Vulkan device memory | Minimal computing power | Once per buffer destruction at [line 14 - Buffer.cpp - destructor] | GPU
Buffer::~Buffer() {
    destroy();
}

// Initialization: Define buffer creation function | Buffer.cpp | Used by Buffer constructor | Creates Vulkan buffer and allocates memory | Buffer Setup - To prepare buffer for data storage | Depends on PhysicalDevice and Vulkan device | Minimal computing power | Once per buffer creation at [line 19 - Buffer.cpp - createBuffer] | GPU
void Buffer::createBuffer(PhysicalDevice& physicalDevice, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = physicalDevice.findMemoryType(
        memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, memory, 0);
}

// Initialization: Define buffer destruction function | Buffer.cpp | Used by Buffer destructor and swap chain recreation | Destroys Vulkan buffer and frees memory | Resource Cleanup - To release GPU resources | Depends on Vulkan device memory | Minimal computing power | Once per buffer destruction at [line 37 - Buffer.cpp - destroy] | GPU
void Buffer::destroy() {
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}
