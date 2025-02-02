#include "CommandBuffer.h"
#include <stdexcept>
#include <array>
#include <iostream>

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool, uint32_t count)
    : device(device), commandPool(commandPool), commandBuffers(count)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

CommandBuffer::~CommandBuffer()
{
    if (!commandBuffers.empty()) {
        vkFreeCommandBuffers(device, commandPool,
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data());
    }
}

// Move constructor
CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
    : device(other.device),
    commandPool(other.commandPool),
    commandBuffers(std::move(other.commandBuffers))
{
    other.commandBuffers.clear();
}

// Move assignment
CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept
{
    if (this != &other) {
        // Free existing buffers
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(device, commandPool,
                static_cast<uint32_t>(commandBuffers.size()),
                commandBuffers.data());
        }
        device = other.device;
        commandPool = other.commandPool;
        commandBuffers = std::move(other.commandBuffers);
        other.commandBuffers.clear();
    }
    return *this;
}

// --------------------------------------------------------------
// 1) recordCommandBuffers -> no index buffer
// --------------------------------------------------------------
void CommandBuffer::recordCommandBuffers(
    VkRenderPass renderPass,
    const std::vector<VkFramebuffer>& framebuffers,
    VkPipeline graphicsPipeline,
    VkExtent2D extent,
    const std::vector<VkDescriptorSet>& descriptorSets,
    VkPipelineLayout pipelineLayout,
    VkBuffer vertexBuffer,
    uint32_t vertexCount
)
{
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
        clearValues[0].color = { { 0.f, 0.f, 0.f, 1.f } };
        clearValues[1].depthStencil = { 1.f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Dynamic viewport
        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

        // Dynamic scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

        // Bind vertex buffer
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        // Bind descriptor set (set=0)
        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, // firstSet=0
            1,
            &descriptorSets[i],
            0, nullptr
        );

        // Draw without index buffer
        vkCmdDraw(commandBuffers[i], vertexCount, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

// --------------------------------------------------------------
// 2) recordCommandBuffersIndexed -> with index buffer
// --------------------------------------------------------------
void CommandBuffer::recordCommandBuffersIndexed(
    VkRenderPass renderPass,
    const std::vector<VkFramebuffer>& framebuffers,
    VkPipeline graphicsPipeline,
    VkExtent2D extent,
    const std::vector<VkDescriptorSet>& descriptorSets,
    VkPipelineLayout pipelineLayout,
    VkBuffer vertexBuffer,
    VkBuffer indexBuffer,
    uint32_t indexCount
)
{
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
        clearValues[0].color = { { 0.f, 0.f, 0.f, 1.f } };
        clearValues[1].depthStencil = { 1.f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

        // Bind vertex buffer
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        // Bind index buffer
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // Bind descriptor set (set=0)
        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, // firstSet=0
            1,
            &descriptorSets[i],
            0, nullptr
        );

        // Draw indexed
        vkCmdDrawIndexed(commandBuffers[i],
            indexCount,
            1,  // instanceCount
            0,  // firstIndex
            0,  // vertexOffset
            0); // firstInstance

        vkCmdEndRenderPass(commandBuffers[i]);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

// --------------------------------------------------------------
// 3) recordCommandBuffersIndexedWithSampler -> index buffer + sampler
// --------------------------------------------------------------
void CommandBuffer::recordCommandBuffersIndexedWithSampler(
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
)
{
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
        clearValues[0].color = { {0.f, 0.f, 0.f, 1.f} };
        clearValues[1].depthStencil = { 1.f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind pipeline
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Dynamic viewport
        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

        // Dynamic scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

        // Bind vertex buffer
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        // Bind index buffer
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // (A) Bind set=0 (UBO)
        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,  // firstSet=0
            1,
            &descriptorSetsUBO[i],
            0, nullptr
        );

        // (B) Bind set=1 (sampler)
        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            1,  // firstSet=1
            1,
            &samplerDescriptorSet,
            0, nullptr
        );

        // Indexed draw
        vkCmdDrawIndexed(commandBuffers[i],
            indexCount,
            1,
            0,
            0,
            0);

        vkCmdEndRenderPass(commandBuffers[i]);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

// --------------------------------------------------------------
// 4) recordCommandBuffersShadowIndexed (NEW) - for shadow pass
// --------------------------------------------------------------
void CommandBuffer::recordCommandBuffersShadowIndexed(
    VkRenderPass shadowRenderPass,
    const std::vector<VkFramebuffer>& shadowFramebuffers,
    VkPipeline shadowPipeline,
    VkExtent2D shadowExtent,
    VkDescriptorSet shadowDescriptorSet, // set=0 for the shadow pipeline
    VkPipelineLayout shadowPipelineLayout,
    VkBuffer vertexBuffer,
    VkBuffer indexBuffer,
    uint32_t indexCount
)
{
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording shadow command buffer!");
        }

        // Begin the shadow render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = shadowRenderPass;
        renderPassInfo.framebuffer = shadowFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = shadowExtent;

        // Clear only depth
        VkClearValue clearDepth{};
        clearDepth.depthStencil = { 1.
            
            
            
            
            , 0 };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearDepth;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind shadow pipeline
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

        // Set dynamic viewport (flip Y)
        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = static_cast<float>(shadowExtent.height);
        viewport.width = static_cast<float>(shadowExtent.width);
        viewport.height = -static_cast<float>(shadowExtent.height); // flip Y
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

        // Set dynamic scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = shadowExtent;
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

        // Optional: depth bias to reduce shadow acne
        vkCmdSetDepthBias(commandBuffers[i], 1.25f, 0.0f, 1.75f);

        // Bind vertex + index buffers
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // Bind descriptor set=0 for the shadow pipeline
        vkCmdBindDescriptorSets(
            commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadowPipelineLayout,
            0, // firstSet=0
            1,
            &shadowDescriptorSet,
            0,
            nullptr
        );

        // Draw
        vkCmdDrawIndexed(commandBuffers[i], indexCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to end shadow command buffer!");
        }
    }
}
