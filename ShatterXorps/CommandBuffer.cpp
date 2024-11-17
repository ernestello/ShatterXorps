// CommandBuffer.cpp

#include "CommandBuffer.h"
#include <stdexcept>
#include <array>

// Initialization: Define CommandBuffer constructor | CommandBuffer.cpp | Used by main.cpp - line where CommandBuffer is instantiated | Allocates command buffers from the pool | Constructor - To prepare command buffers for recording | Depends on Vulkan device and CommandPool | Minimal computing power | Once per command buffer creation at [line 6 - CommandBuffer.cpp - constructor] | CPU
CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, size_t bufferCount)
    : device(device), commandPool(commandPool), commandBuffers(bufferCount) {
    createCommandBuffers(bufferCount);
}

// Initialization: Define CommandBuffer destructor | CommandBuffer.cpp | Used by main.cpp - line where CommandBuffer is destroyed | Frees command buffers | Destructor - To release command buffer resources | Depends on Vulkan device and CommandPool | Minimal computing power | Once per command buffer destruction at [line 12 - CommandBuffer.cpp - destructor] | CPU
CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
}

// Initialization: Define command buffer creation function | CommandBuffer.cpp | Used by CommandBuffer constructor | Allocates Vulkan command buffers | Command Buffer Allocation - To record rendering commands | Depends on Vulkan device and CommandPool | Minimal computing power | Once per buffer allocation at [line 17 - CommandBuffer.cpp - createCommandBuffers] | CPU
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

// Initialization: Define command buffer recording function | CommandBuffer.cpp | Used by main.cpp - line where command buffers are recorded | Records rendering commands into each command buffer | Command Recording - To define rendering operations | Depends on RenderPass, SwapChain, GraphicsPipeline, DescriptorSets, VertexBuffer | High computing power | Once per frame at [line 27 - CommandBuffer.cpp - recordCommandBuffers] | CPU, GPU
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
        clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer, offsets);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
            0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}
