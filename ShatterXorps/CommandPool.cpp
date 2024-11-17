// CommandPool.cpp

#include "CommandPool.h"

// Initialization: Define CommandPool constructor | CommandPool.cpp | Used by main.cpp - line where CommandPool is instantiated | Creates a Vulkan command pool | Constructor - To allocate command buffers | Depends on Vulkan device and queue family | Minimal computing power | Once at [line 4 - CommandPool.cpp - constructor] | CPU
CommandPool::CommandPool(VkDevice device, uint32_t queueFamilyIndex)
    : device(device), commandPool(VK_NULL_HANDLE) {
    createCommandPool(queueFamilyIndex);
}

// Initialization: Define CommandPool destructor | CommandPool.cpp | Used by main.cpp - line where CommandPool is destroyed | Destroys Vulkan command pool | Destructor - To release command pool resources | Depends on Vulkan device memory | Minimal computing power | Once at [line 10 - CommandPool.cpp - destructor] | CPU
CommandPool::~CommandPool() {
    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }
}

// Initialization: Define command pool creation function | CommandPool.cpp | Used by CommandPool constructor | Creates Vulkan command pool with specified queue family | Command Pool Setup - To manage command buffers | Depends on queue family index and Vulkan device | Minimal computing power | Once at [line 14 - CommandPool.cpp - createCommandPool] | CPU
void CommandPool::createCommandPool(uint32_t queueFamilyIndex) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}
