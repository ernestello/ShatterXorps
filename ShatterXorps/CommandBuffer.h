// CommandBuffer.h
#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

class CommandBuffer {
public:
    CommandBuffer(VkDevice device, VkCommandPool commandPool, uint32_t count);
    ~CommandBuffer();

    // Delete copy constructor and copy assignment operator
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    // Move constructor and move assignment operator
    CommandBuffer(CommandBuffer&& other) noexcept;
    CommandBuffer& operator=(CommandBuffer&& other) noexcept;

    // Accessor
    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return commandBuffers; }

    // Function to record command buffers
    void recordCommandBuffers(
        VkRenderPass renderPass,
        const std::vector<VkFramebuffer>& framebuffers,
        VkPipeline graphicsPipeline,
        VkExtent2D extent,
        const std::vector<VkDescriptorSet>& descriptorSets,
        VkPipelineLayout pipelineLayout,
        VkBuffer vertexBuffer,
        uint32_t vertexCount // Added parameter
    );

private:
    VkDevice device;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

#endif // COMMAND_BUFFER_H
