// main.cpp
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS            // Forces all angle calculations to use radians.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Makes the projection matrix suitable for Vulkan's coordinate system.
#define GLM_ENABLE_EXPERIMENTAL      // Enable string casting for debugging
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp> // For glm::to_string
#include <GLFW/glfw3.h>

#include "VulkanInstance.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "GraphicsPipeline.h"
#include "Buffer.h"
#include "Vertex.h" // Include the Vertex header
#include "UniformBufferObject.h" // **Added Include**

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring> // For memset
#include <chrono>  // For timing
#include <sstream> // For window title updates
#include <optional>

#include <cassert> // For assertions

// Forward declarations for cleanup
void cleanupSwapChain(
    VkDevice device,
    SwapChain& swapChain,
    RenderPass& renderPass,
    GraphicsPipeline& graphicsPipeline,
    std::vector<Buffer>& uniformBuffers,
    VkDescriptorPool& descriptorPool, // Passed by reference
    Buffer& vertexBuffer,
    CommandBuffer& commandBuffer
);

// Function to recreate the swapchain
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
    CommandPool& commandPool // Added parameter
) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    // Cleanup existing swapchain and related resources
    cleanupSwapChain(device, swapChain, renderPass, graphicsPipeline, uniformBuffers, descriptorPool, vertexBuffer, commandBuffer);

    // Recreate swapchain
    SwapChain newSwapChain(physicalDevice, device, swapChain.getSurface(), window);
    swapChain = std::move(newSwapChain);
    std::cout << "SwapChain recreated successfully." << std::endl;

    // Recreate render pass
    RenderPass newRenderPass(device, physicalDevice.getPhysicalDevice(), swapChain.getSwapChainImageFormat());
    renderPass = std::move(newRenderPass);
    std::cout << "Render pass recreated successfully." << std::endl;

    // Recreate graphics pipeline
    GraphicsPipeline newGraphicsPipeline(device, swapChain.getSwapChainExtent(), renderPass.getRenderPass());
    graphicsPipeline = std::move(newGraphicsPipeline);
    std::cout << "Graphics pipeline recreated successfully." << std::endl;

    // Recreate framebuffers
    swapChain.createFramebuffers(renderPass.getRenderPass());
    std::cout << "Framebuffers recreated successfully." << std::endl;

    // Recreate uniform buffers
    size_t bufferCount = swapChain.getFramebuffers().size();
    uniformBuffers.clear();
    uniformBuffers.reserve(bufferCount);
    for (size_t i = 0; i < bufferCount; i++) {
        uniformBuffers.emplace_back(device, physicalDevice, sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    std::cout << "Uniform buffers recreated successfully." << std::endl;

    // Recreate descriptor pool
    VkDescriptorPoolCreateInfo poolInfoDesc{};
    poolInfoDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(bufferCount);
    poolInfoDesc.poolSizeCount = 1;
    poolInfoDesc.pPoolSizes = &poolSize;
    poolInfoDesc.maxSets = static_cast<uint32_t>(bufferCount);

    if (vkCreateDescriptorPool(device, &poolInfoDesc, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to recreate descriptor pool!");
    }
    std::cout << "Descriptor pool recreated successfully." << std::endl;

    // Allocate descriptor sets
    std::vector<VkDescriptorSet> descriptorSets(bufferCount);
    std::vector<VkDescriptorSetLayout> layouts(bufferCount, graphicsPipeline.getDescriptorSetLayout());

    VkDescriptorSetAllocateInfo allocInfoDesc{};
    allocInfoDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfoDesc.descriptorPool = descriptorPool;
    allocInfoDesc.descriptorSetCount = static_cast<uint32_t>(bufferCount);
    allocInfoDesc.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device, &allocInfoDesc, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets during swapchain recreation!");
    }
    std::cout << "Descriptor sets allocated successfully during swapchain recreation." << std::endl;

    // Update descriptor sets
    for (size_t i = 0; i < bufferCount; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i].buffer;
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
    std::cout << "Descriptor sets updated successfully during swapchain recreation." << std::endl;

    // Reset the command pool to reuse it
    vkResetCommandPool(device, commandPool.getCommandPool(), 0);
    std::cout << "Command pool reset successfully during swapchain recreation." << std::endl;

    // Recreate command buffers
    commandBuffer = CommandBuffer(device, commandPool.getCommandPool(), bufferCount);
    commandBuffer.recordCommandBuffers(
        renderPass.getRenderPass(),
        swapChain.getFramebuffers(),
        graphicsPipeline.getPipeline(),
        swapChain.getSwapChainExtent(),
        descriptorSets,
        graphicsPipeline.getPipelineLayout(),
        vertexBuffer.buffer // Pass the vertex buffer
    );
    std::cout << "Command buffers recorded successfully during swapchain recreation." << std::endl;
}

int main() {
    try {
        // 1. Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW!\n";
            return EXIT_FAILURE;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Prevents GLFW from creating an OpenGL context

        // 2. Create a GLFW window
        int width = 800;
        int height = 600;
        GLFWwindow* window = glfwCreateWindow(width, height, "My Vulkan App", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window!");
        }

        // 3. Retrieve required Vulkan extensions from GLFW
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // Convert to a std::vector for easier handling
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // 4. Create Vulkan instance with the required extensions
        VulkanInstance vulkanInstance("My Vulkan App", VK_MAKE_VERSION(1, 0, 0), extensions);
        VkInstance instance = vulkanInstance.getInstance();
        std::cout << "Vulkan instance created successfully.\n";

        // 5. Create Vulkan surface for the window
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan surface!");
        }

        // 6. Enumerate Physical Devices and Create Logical Device
        PhysicalDevice physicalDevice(instance, surface); // Pass the surface
        VkPhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();

        std::cout << "Selected GPU: " << deviceProperties.deviceName << "\n";

        // 7. After creating the Vulkan device
        VkDevice device = physicalDevice.getDevice();
        std::cout << "Logical device created successfully." << std::endl;

        // 8. Initialize SwapChain
        SwapChain swapChain(physicalDevice, device, surface, window);
        std::cout << "SwapChain initialized successfully." << std::endl;

        // 9. Create Render Pass
        RenderPass renderPass(device, physicalDevice.getPhysicalDevice(), swapChain.getSwapChainImageFormat());
        std::cout << "Render pass created successfully." << std::endl;

        // 10. Create Graphics Pipeline
        GraphicsPipeline graphicsPipeline(device, swapChain.getSwapChainExtent(), renderPass.getRenderPass());
        std::cout << "Graphics pipeline created successfully." << std::endl;

        // 11. Create Framebuffers
        swapChain.createFramebuffers(renderPass.getRenderPass());
        std::cout << "Framebuffers created successfully." << std::endl;

        // 12. Create Command Pool
        CommandPool commandPool(device, physicalDevice.getGraphicsQueueFamilyIndex());
        std::cout << "Command Pool created successfully." << std::endl;

        // 13. Retrieve queues
        VkQueue graphicsQueue;
        vkGetDeviceQueue(device, physicalDevice.getGraphicsQueueFamilyIndex(), 0, &graphicsQueue);
        VkQueue presentQueue;
        vkGetDeviceQueue(device, physicalDevice.getPresentQueueFamilyIndex(), 0, &presentQueue);
        std::cout << "Queues retrieved successfully." << std::endl;

        // 14. Create synchronization objects
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so the first frame can be rendered

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects!");
        }
        std::cout << "Synchronization objects created successfully." << std::endl;

        // 15. Create Uniform Buffers
        size_t bufferCount = swapChain.getFramebuffers().size();
        std::vector<Buffer> uniformBuffers;
        uniformBuffers.reserve(bufferCount);
        for (size_t i = 0; i < bufferCount; i++) {
            uniformBuffers.emplace_back(device, physicalDevice, sizeof(UniformBufferObject),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }

        // 16. Create Descriptor Pool
        VkDescriptorPool descriptorPool;
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(bufferCount);

        VkDescriptorPoolCreateInfo poolInfoDesc{};
        poolInfoDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfoDesc.poolSizeCount = 1;
        poolInfoDesc.pPoolSizes = &poolSize;
        poolInfoDesc.maxSets = static_cast<uint32_t>(bufferCount);

        if (vkCreateDescriptorPool(device, &poolInfoDesc, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }

        // 17. Allocate Descriptor Sets
        std::vector<VkDescriptorSet> descriptorSets(bufferCount);
        std::vector<VkDescriptorSetLayout> layouts(bufferCount, graphicsPipeline.getDescriptorSetLayout());

        VkDescriptorSetAllocateInfo allocInfoDesc{};
        allocInfoDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfoDesc.descriptorPool = descriptorPool;
        allocInfoDesc.descriptorSetCount = static_cast<uint32_t>(bufferCount);
        allocInfoDesc.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device, &allocInfoDesc, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        // 18. Update Descriptor Sets
        for (size_t i = 0; i < bufferCount; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].buffer;
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

        // **Define Vertex Data**
        const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Bottom-left vertex (Red)
            {{0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}}, // Bottom-right vertex (Green)
            {{0.0f, 0.5f},   {0.0f, 0.0f, 1.0f}}  // Top vertex (Blue)
        };

        // **Create the Vertex Buffer**
        Buffer vertexBuffer(device, physicalDevice, sizeof(vertices[0]) * vertices.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // Copy vertex data to the buffer
        void* vertexData;
        VkResult mapResult = vkMapMemory(device, vertexBuffer.memory, 0, vertexBuffer.size, 0, &vertexData);
        if (mapResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to map vertex buffer memory!");
        }
        memcpy(vertexData, vertices.data(), static_cast<size_t>(vertexBuffer.size));
        vkUnmapMemory(device, vertexBuffer.memory);

        // 19. Create Command Buffers
        CommandBuffer commandBufferObj(device, commandPool.getCommandPool(), bufferCount);
        std::cout << "Command Buffers created successfully." << std::endl;

        // 20. Record Command Buffers
        commandBufferObj.recordCommandBuffers(
            renderPass.getRenderPass(),
            swapChain.getFramebuffers(),
            graphicsPipeline.getPipeline(),
            swapChain.getSwapChainExtent(),
            descriptorSets,
            graphicsPipeline.getPipelineLayout(),
            vertexBuffer.buffer // Pass the vertex buffer
        );
        std::cout << "Command Buffers recorded successfully." << std::endl;

        // **Timing for Uniform Buffer Updates**
        auto startTime = std::chrono::high_resolution_clock::now();

        // **Timing variables for FPS calculation**
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
                // Recreate the swapchain
                recreateSwapChain(window, device, physicalDevice, swapChain, renderPass, graphicsPipeline, uniformBuffers, descriptorPool, vertexBuffer, commandBufferObj, commandPool);
                continue;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw std::runtime_error("Failed to acquire swap chain image!");
            }

            // 3. Reset the fence for the current frame
            vkResetFences(device, 1, &inFlightFence);

            // **Update the uniform buffer for the current frame**
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

            if (uniformBuffers[imageIndex].memory == VK_NULL_HANDLE) {
                throw std::runtime_error("Uniform buffer memory is null!");
            }

            // Map memory
            void* data;
            VkResult mapResultUBO = vkMapMemory(device, uniformBuffers[imageIndex].memory, 0, sizeof(ubo), 0, &data);
            if (mapResultUBO != VK_SUCCESS) {
                throw std::runtime_error("Failed to map uniform buffer memory!");
            }
            memcpy(data, &ubo, sizeof(ubo));
            vkUnmapMemory(device, uniformBuffers[imageIndex].memory);

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
                // Recreate the swapchain
                recreateSwapChain(window, device, physicalDevice, swapChain, renderPass, graphicsPipeline, uniformBuffers, descriptorPool, vertexBuffer, commandBufferObj, commandPool);
                continue;
            }
            else if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to present swap chain image!");
            }

            // **FPS Counter**
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
        std::cout << "Idle device." << std::endl;

        // 6. Cleanup synchronization objects
        vkDestroyFence(device, inFlightFence, nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        std::cout << "Synchronization objects cleaned up." << std::endl;

        // 7. Clean up uniform buffers and descriptor pool
        for (auto& uniformBuffer : uniformBuffers) {
            uniformBuffer.destroy(device); // Use destroy() instead of cleanup()
        }
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        // **Destroy the vertex buffer**
        vertexBuffer.destroy(device);

        // 8. Clean up remaining Vulkan objects
        cleanupSwapChain(device, swapChain, renderPass, graphicsPipeline, uniformBuffers, descriptorPool, vertexBuffer, commandBufferObj);
        std::cout << "Swapchain and related resources cleaned up." << std::endl;

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "Vulkan objects cleaned up." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // vkDestroyInstance is automatically called when vulkanInstance goes out of scope
    std::cout << "Vulkan instance destroyed successfully.\n";
    return EXIT_SUCCESS;
}

void cleanupSwapChain(
    VkDevice device,
    SwapChain& swapChain,
    RenderPass& renderPass,
    GraphicsPipeline& graphicsPipeline,
    std::vector<Buffer>& uniformBuffers,
    VkDescriptorPool& descriptorPool,
    Buffer& vertexBuffer,
    CommandBuffer& commandBuffer
) {
    // Wait for the device to be idle before cleanup
    vkDeviceWaitIdle(device);

    // Destroy the descriptor pool
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;

    // Destroy uniform buffers
    for (auto& uniformBuffer : uniformBuffers) {
        uniformBuffer.destroy(device);
    }
    uniformBuffers.clear();

    // Destroy graphics pipeline and render pass
    graphicsPipeline.destroy(device);
    renderPass.destroy(device);

    // Destroy swapchain and its framebuffers
    swapChain.destroy(device);

    // Destroy the vertex buffer
    vertexBuffer.destroy(device);

    // Command buffers are already reset and re-recorded in recreateSwapChain
}
