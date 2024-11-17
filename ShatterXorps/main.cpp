// main.cpp
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanInstance.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Vertex.h"
#include "Buffer.h"
#include "UniformBufferObject.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <cstring>
#include <set>
#include <fstream>
#include <array>
#include <algorithm>
#include <sstream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Vertex 1: position and color
    {{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}}, // Vertex 2: position and color
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}  // Vertex 3: position and color
};

GLFWwindow* initWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Window", nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("Failed to create GLFW window!");
    }

    return window;
}

void recreateSwapChain(
    GLFWwindow* window,
    VkDevice device,
    PhysicalDevice& physicalDevice,
    SwapChain& swapChain,
    RenderPass& renderPass,
    GraphicsPipeline& graphicsPipeline,
    std::vector<Buffer>& uniformBuffers,
    VkDescriptorPool& descriptorPool,
    Buffer& vertexBuffer,
    CommandBuffer& commandBuffer,
    VkCommandPool commandPool
) {
    vkDeviceWaitIdle(device);

    swapChain.destroy();
    renderPass.~RenderPass();
    graphicsPipeline.~GraphicsPipeline();

    swapChain = SwapChain(physicalDevice, device, swapChain.getSurface(), window);
    renderPass = RenderPass(device, physicalDevice.getPhysicalDevice(), swapChain.getSwapChainImageFormat());
    graphicsPipeline = GraphicsPipeline(device, swapChain.getSwapChainExtent(), renderPass.getRenderPass());

    swapChain.createFramebuffers(renderPass.getRenderPass());

    // Recreate uniform buffers and descriptor sets
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // Destroy old uniform buffers
    for (auto& uniformBuffer : uniformBuffers) {
        uniformBuffer.destroy();
    }
    uniformBuffers.clear();

    uniformBuffers.reserve(swapChain.getSwapChainImages().size());
    for (size_t i = 0; i < swapChain.getSwapChainImages().size(); i++) {
        uniformBuffers.emplace_back(
            device,
            physicalDevice,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
    }


    // Recreate descriptor pool and descriptor sets
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(uniformBuffers.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(uniformBuffers.size());

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    std::vector<VkDescriptorSet> descriptorSets(uniformBuffers.size());

    std::vector<VkDescriptorSetLayout> layouts(uniformBuffers.size(), graphicsPipeline.getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(uniformBuffers.size());
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i].getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    // Re-record command buffers
    commandBuffer.recordCommandBuffers(
        renderPass.getRenderPass(),
        swapChain.getFramebuffers(),
        graphicsPipeline.getPipeline(),
        swapChain.getSwapChainExtent(),
        descriptorSets,
        graphicsPipeline.getPipelineLayout(),
        vertexBuffer.getBuffer()
    );
}

int main() {
    GLFWwindow* window = initWindow();

    try {
        // 1. Create Vulkan instance
        std::vector<const char*> extensions;
        VulkanInstance vulkanInstance("My Vulkan App", VK_MAKE_VERSION(1, 0, 0), extensions);
        VkInstance instance = vulkanInstance.getInstance();
        std::cout << "Vulkan instance created successfully." << std::endl;

        // 2. Create Vulkan surface
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
        std::cout << "Window surface created successfully." << std::endl;

        // 3. Select physical device and create logical device
        PhysicalDevice physicalDevice(instance, surface);
        VkDevice device = physicalDevice.getDevice();
        std::cout << "Physical and logical devices created successfully." << std::endl;

        // 4. Create swap chain
        SwapChain swapChain(physicalDevice, device, surface, window);
        std::cout << "Swap chain created successfully." << std::endl;

        // 5. Create render pass
        RenderPass renderPass(device, physicalDevice.getPhysicalDevice(), swapChain.getSwapChainImageFormat());
        std::cout << "Render pass created successfully." << std::endl;

        // 6. Create graphics pipeline
        GraphicsPipeline graphicsPipeline(device, swapChain.getSwapChainExtent(), renderPass.getRenderPass());
        std::cout << "Graphics pipeline created successfully." << std::endl;

        // 7. Create framebuffers
        swapChain.createFramebuffers(renderPass.getRenderPass());
        std::cout << "Framebuffers created successfully." << std::endl;

        // 8. Create command pool
        CommandPool commandPool(device, physicalDevice.getGraphicsQueueFamilyIndex());
        std::cout << "Command pool created successfully." << std::endl;

        // 9. Create vertex buffer
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        Buffer vertexBuffer(
            device,
            physicalDevice,
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // Map memory and copy vertex data
        void* data;
        vkMapMemory(device, vertexBuffer.getMemory(), 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, vertexBuffer.getMemory());

        std::cout << "Vertex buffer created successfully." << std::endl;

        // 10. Create uniform buffers
        VkDeviceSize uniformBufferSize = sizeof(UniformBufferObject);
        std::vector<Buffer> uniformBuffers;
        uniformBuffers.reserve(swapChain.getSwapChainImages().size());

        for (size_t i = 0; i < swapChain.getSwapChainImages().size(); i++) {
            uniformBuffers.emplace_back(
                device,
                physicalDevice,
                uniformBufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
        std::cout << "Uniform buffers created successfully." << std::endl;


        // 11. Create descriptor pool
        VkDescriptorPool descriptorPool;

        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(uniformBuffers.size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(uniformBuffers.size());

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
        std::cout << "Descriptor pool created successfully." << std::endl;

        // 12. Allocate descriptor sets
        std::vector<VkDescriptorSet> descriptorSets(uniformBuffers.size());

        std::vector<VkDescriptorSetLayout> layouts(uniformBuffers.size(), graphicsPipeline.getDescriptorSetLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(uniformBuffers.size());
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < uniformBuffers.size(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].getBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
        std::cout << "Descriptor sets allocated and updated successfully." << std::endl;

        // 13. Create command buffers
        CommandBuffer commandBufferObj(device, commandPool.getCommandPool(), swapChain.getSwapChainImages().size());
        std::cout << "Command buffers created successfully." << std::endl;

        // 14. Record command buffers
        commandBufferObj.recordCommandBuffers(
            renderPass.getRenderPass(),
            swapChain.getFramebuffers(),
            graphicsPipeline.getPipeline(),
            swapChain.getSwapChainExtent(),
            descriptorSets,
            graphicsPipeline.getPipelineLayout(),
            vertexBuffer.getBuffer()
        );
        std::cout << "Command buffers recorded successfully." << std::endl;

        // 15. Create synchronization objects
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects!");
        }
        std::cout << "Synchronization objects created successfully." << std::endl;

        VkQueue graphicsQueue;
        vkGetDeviceQueue(device, physicalDevice.getGraphicsQueueFamilyIndex(), 0, &graphicsQueue);

        VkQueue presentQueue;
        vkGetDeviceQueue(device, physicalDevice.getPresentQueueFamilyIndex(), 0, &presentQueue);

        // Timing for uniform buffer updates
        auto startTime = std::chrono::high_resolution_clock::now();

        // Timing variables for FPS calculation
        auto fpsStartTime = std::chrono::high_resolution_clock::now();
        int frameCount = 0;

        // Main loop
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Handle window resize
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
                // If window is minimized, wait until it is restored
                while (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
                    glfwWaitEvents();
                }
            }

            // 1. Wait for the previous frame to finish
            vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

            // 2. Acquire the next image from the swap chain
            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(device, swapChain.getSwapChain(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                // Recreate the swap chain
                recreateSwapChain(window, device, physicalDevice, swapChain, renderPass, graphicsPipeline, uniformBuffers, descriptorPool, vertexBuffer, commandBufferObj, commandPool.getCommandPool());
                continue;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw std::runtime_error("Failed to acquire swap chain image!");
            }

            // 3. Reset the fence for the current frame
            vkResetFences(device, 1, &inFlightFence);

            // Update the uniform buffer for the current frame
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            UniformBufferObject ubo{};
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around Z-axis
            ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), // Camera position
                glm::vec3(0.0f, 0.0f, 0.0f), // Look at origin
                glm::vec3(0.0f, 0.0f, 1.0f)); // Up vector
            ubo.proj = glm::perspective(glm::radians(45.0f),
                swapChain.getSwapChainExtent().width / (float)swapChain.getSwapChainExtent().height,
                0.1f, 10.0f);
            ubo.proj[1][1] *= -1; // Flip Y for Vulkan's coordinate system

            // Validate imageIndex
            if (imageIndex >= uniformBuffers.size()) {
                throw std::runtime_error("imageIndex out of bounds!");
            }

            // Map memory
            void* uboData;
            vkMapMemory(device, uniformBuffers[imageIndex].getMemory(), 0, sizeof(ubo), 0, &uboData);
            memcpy(uboData, &ubo, sizeof(ubo));
            vkUnmapMemory(device, uniformBuffers[imageIndex].getMemory());

            // 4. Submit the command buffer
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            VkCommandBuffer commandBuffersToSubmit[] = { commandBufferObj.getCommandBuffers()[imageIndex] };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = commandBuffersToSubmit;

            VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
                throw std::runtime_error("Failed to submit draw command buffer!");
            }

            // 5. Present the image
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            VkSwapchainKHR swapChains[] = { swapChain.getSwapChain() };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;

            result = vkQueuePresentKHR(presentQueue, &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                // Recreate the swap chain
                recreateSwapChain(window, device, physicalDevice, swapChain, renderPass, graphicsPipeline, uniformBuffers, descriptorPool, vertexBuffer, commandBufferObj, commandPool.getCommandPool());
                continue;
            }
            else if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to present swap chain image!");
            }

            // FPS Counter
            frameCount++;

            auto fpsCurrentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> fpsElapsedTime = fpsCurrentTime - fpsStartTime;

            if (fpsElapsedTime.count() >= 1.0f) {
                float fps = frameCount / fpsElapsedTime.count();

                // Output FPS to console
                std::cout << "FPS: " << fps << std::endl;

                // Set the window title to display FPS
                std::ostringstream windowTitle;
                windowTitle << "My Vulkan App - FPS: " << static_cast<int>(fps);
                glfwSetWindowTitle(window, windowTitle.str().c_str());

                // Reset counters
                fpsStartTime = fpsCurrentTime;
                frameCount = 0;
            }
        }

        vkDeviceWaitIdle(device);
        std::cout << "Device idle. Cleaning up resources..." << std::endl;

        // Cleanup synchronization objects
        vkDestroyFence(device, inFlightFence, nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        std::cout << "Synchronization objects cleaned up." << std::endl;

        // Cleanup uniform buffers and descriptor pool
        for (auto& uniformBuffer : uniformBuffers) {
            uniformBuffer.destroy();
        }
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        // The vertex buffer will be destroyed in its destructor

        // Cleanup swap chain and related resources
        swapChain.destroy();
        renderPass.~RenderPass();
        graphicsPipeline.~GraphicsPipeline();
        commandPool.~CommandPool();
        std::cout << "Swap chain and related resources cleaned up." << std::endl;

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "Vulkan objects cleaned up." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // vkDestroyInstance is automatically called when vulkanInstance goes out of scope
    std::cout << "Vulkan instance destroyed successfully.\n";
    return EXIT_SUCCESS;
}
