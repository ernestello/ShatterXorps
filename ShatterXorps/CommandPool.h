#ifndef COMMAND_POOL_H
#define COMMAND_POOL_H

#include <vulkan/vulkan.h>
#include <stdexcept>

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
