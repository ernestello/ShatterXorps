// Buffer.cpp
#include "Buffer.h"
#include <stdexcept>

Buffer::Buffer(VkDevice device, PhysicalDevice& physicalDevice, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : device(device), buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), size(size) {
    createBuffer(physicalDevice, size, usage, properties);
}

Buffer::~Buffer() {
    destroy();
}

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
