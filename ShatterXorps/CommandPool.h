// CommandPool.h
#ifndef COMMAND_POOL_H
#define COMMAND_POOL_H

#include <vulkan/vulkan.h>
#include <stdexcept>

// Initialization: Define CommandPool class | CommandPool.h | Used by main.cpp and other classes | Encapsulates Vulkan command pool functionality | Class Definition - To manage command pool resources | Depends on Vulkan device and queue family | Minimal computing power | Defined once at [line 4 - CommandPool.h - global scope] | CPU
class CommandPool {
public:
    CommandPool(VkDevice device, uint32_t queueFamilyIndex);
    ~CommandPool();

    VkCommandPool getCommandPool() const { return commandPool; }

private:
    VkDevice device;
    VkCommandPool commandPool;

    void createCommandPool(uint32_t queueFamilyIndex);
};

#endif // COMMAND_POOL_H
