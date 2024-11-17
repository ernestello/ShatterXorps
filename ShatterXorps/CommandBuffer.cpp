// CommandBuffer.cpp
#include "CommandBuffer.h"
#include <array> // **Ensure <array> is included**

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, size_t bufferCount)
    : device(device), commandPool(commandPool), commandBuffers(bufferCount, VK_NULL_HANDLE) {
    createCommandBuffers(bufferCount);
}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
}

void CommandBuffer::createCommandBuffers(size_t bufferCount) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(bufferCount);

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void CommandBuffer::recordCommandBuffers(
    VkRenderPass renderPass,
    const std::vector<VkFramebuffer>& framebuffers,
    VkPipeline graphicsPipeline,
    VkExtent2D extent,
    const std::vector<VkDescriptorSet>& descriptorSets,
    VkPipelineLayout pipelineLayout,
    VkBuffer vertexBuffer
) {
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;

        // Define clear values for color and depth attachments
        std::array<VkClearValue, 2> clearValues = {
            VkClearValue{ { 1.0f, 2.0f, 2.0f, 1.0f } }, // Attachment 0: Color
            VkClearValue{ { 1.0f, 0 } }                // Attachment 1: Depth
        };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkBuffer vertexBuffersArray[] = { vertexBuffer };
        VkDeviceSize offsetsArray[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffersArray, offsetsArray);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
            0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}
