#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>

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

    // Accessor for the command buffers
    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return commandBuffers; }

    //--------------------------------------------------------------
    // 1) recordCommandBuffers -> No index buffer
    //--------------------------------------------------------------
    void recordCommandBuffers(
        VkRenderPass renderPass,
        const std::vector<VkFramebuffer>& framebuffers,
        VkPipeline graphicsPipeline,
        VkExtent2D extent,
        const std::vector<VkDescriptorSet>& descriptorSets, // e.g. set=0
        VkPipelineLayout pipelineLayout,
        VkBuffer vertexBuffer,
        uint32_t vertexCount
    );

    //--------------------------------------------------------------
    // 2) recordCommandBuffersIndexed -> Has index buffer
    //--------------------------------------------------------------
    void recordCommandBuffersIndexed(
        VkRenderPass renderPass,
        const std::vector<VkFramebuffer>& framebuffers,
        VkPipeline graphicsPipeline,
        VkExtent2D extent,
        const std::vector<VkDescriptorSet>& descriptorSets, // e.g. set=0
        VkPipelineLayout pipelineLayout,
        VkBuffer vertexBuffer,
        VkBuffer indexBuffer,
        uint32_t indexCount
    );

    //--------------------------------------------------------------
    // 3) recordCommandBuffersIndexedWithSampler -> index buffer + sampler
    //--------------------------------------------------------------
    void recordCommandBuffersIndexedWithSampler(
        VkRenderPass renderPass,
        const std::vector<VkFramebuffer>& framebuffers,
        VkPipeline graphicsPipeline,
        VkExtent2D extent,
        const std::vector<VkDescriptorSet>& descriptorSetsUBO, // set=0
        VkDescriptorSet samplerDescriptorSet,                  // set=1
        VkPipelineLayout pipelineLayout,
        VkBuffer vertexBuffer,
        VkBuffer indexBuffer,
        uint32_t indexCount
    );

    //--------------------------------------------------------------
    // 4) recordCommandBuffersShadowIndexed (NEW)
    //    For a shadow pass that uses set=0 with an index buffer
    //--------------------------------------------------------------
    void recordCommandBuffersShadowIndexed(
        VkRenderPass shadowRenderPass,
        const std::vector<VkFramebuffer>& shadowFramebuffers,
        VkPipeline shadowPipeline,
        VkExtent2D shadowExtent,
        VkDescriptorSet shadowDescriptorSet, // typically set=0 for "light MVP"
        VkPipelineLayout shadowPipelineLayout,
        VkBuffer vertexBuffer,
        VkBuffer indexBuffer,
        uint32_t indexCount
    );

private:
    VkDevice device;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

#endif // COMMAND_BUFFER_H
