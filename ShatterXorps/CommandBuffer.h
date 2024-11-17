// CommandBuffer.h
#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <array> // **Added Include**
#include <stdexcept>
#include "CommandPool.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "Buffer.h"

class CommandBuffer {
public:
    CommandBuffer(VkDevice device, VkCommandPool commandPool, size_t bufferCount);
    ~CommandBuffer();

    void recordCommandBuffers(
        VkRenderPass renderPass,
        const std::vector<VkFramebuffer>& framebuffers,
        VkPipeline graphicsPipeline,
        VkExtent2D extent,
        const std::vector<VkDescriptorSet>& descriptorSets,
        VkPipelineLayout pipelineLayout,
        VkBuffer vertexBuffer
    );

    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return commandBuffers; }

private:
    VkDevice device;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    void createCommandBuffers(size_t bufferCount);
};

#endif // COMMAND_BUFFER_H