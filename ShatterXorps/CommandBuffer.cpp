// CommandBuffer.cpp
#include "CommandBuffer.h"
#include <stdexcept>
#include <array>
#include <iostream>

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, uint32_t count)
    : device(device), commandPool(commandPool), commandBuffers(count) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

CommandBuffer::~CommandBuffer() {
    if (!commandBuffers.empty()) {
        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    }
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
    : device(other.device), commandPool(other.commandPool), commandBuffers(std::move(other.commandBuffers)) {
    other.commandBuffers.clear();
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept {
    if (this != &other) {
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        }

        device = other.device;
        commandPool = other.commandPool;
        commandBuffers = std::move(other.commandBuffers);

        other.commandBuffers.clear();
    }
    return *this;
}

void CommandBuffer::recordCommandBuffers(
    VkRenderPass renderPass,
    const std::vector<VkFramebuffer>& framebuffers,
    VkPipeline graphicsPipeline,
    VkExtent2D extent,
    const std::vector<VkDescriptorSet>& descriptorSets,
    VkPipelineLayout pipelineLayout,
    VkBuffer vertexBuffer,
    uint32_t vertexCount // Added parameter
) {
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Set dynamic viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

        // Set dynamic scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
            0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDraw(commandBuffers[i], vertexCount, 1, 0, 0); // Correct number of arguments

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}
