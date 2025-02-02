// IMPORTANT: save file as "UTF-8 without BOM" so that the very first character is '#':
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "VulkanInstance.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "ShadowPipeline.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Vertex.h"
#include "Buffer.h"
#include "UniformBufferObject.h"
#include "CubeVertices.h"
#include "PixelTracer.h"
#include "CylinderMesh.h"
#include "LightRayPipeline.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstring>
#include <fstream>
#include <array>
#include <algorithm>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <set>

// -------------------------------------------------------------------------------------
// Variables for "selected object" and camera speed, etc.
// -------------------------------------------------------------------------------------
static glm::vec3 g_selectedObjectPos = glm::vec3(0.0f, 1.0f, 0.0f);
static bool      g_objectIsSelected = false;
static bool      g_showMouseCursor = false;
static float     g_cameraMoveSpeed = 2.5f;

static bool framebufferResized = false;

// Basic camera
static glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
static glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
static glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

static float yaw = -90.0f;
static float pitch = 0.0f;
static float lastX = 800.f / 2.f;
static float lastY = 600.f / 2.f;
static bool  firstMouse = true;

// For resizing
void framebufferResizeCallback(GLFWwindow* window, int w, int h) {
    framebufferResized = true;
}

// For mouse movement
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    // If the cursor is in "UI mode" (CTRL held => normal cursor), skip camera rotation
    if (g_showMouseCursor) {
        return;
    }

    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
    front.y = sinf(glm::radians(pitch));
    front.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// A small plane that we treat as the “menu plane” at x=3
static const std::vector<Vertex> planeVertices = {
    // position            color              normal
    {{3.f, -1.f, -1.f},   {0.8f, 0.2f, 0.2f}, {0.f, 1.f, 0.f}},
    {{3.f,  1.f, -1.f},   {0.8f, 0.2f, 0.2f}, {0.f, 1.f, 0.f}},
    {{3.f,  1.f,  1.f},   {0.8f, 0.2f, 0.2f}, {0.f, 1.f, 0.f}},
    {{3.f, -1.f,  1.f},   {0.8f, 0.2f, 0.2f}, {0.f, 1.f, 0.f}},
};
static const std::vector<uint16_t> planeIndices = { 0,1,2, 2,3,0 };

// Minimal struct for “shadow pass”
struct ShadowResources {
    VkImage        depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkImageView    depthView = VK_NULL_HANDLE;
    VkFramebuffer  framebuffer = VK_NULL_HANDLE;
    VkExtent2D     extent;
};

void recordShadowCommandBuffer(
    VkCommandBuffer   cmd,
    VkRenderPass      shadowRenderPass,
    VkFramebuffer     shadowFramebuffer,
    VkExtent2D        shadowExtent,
    VkPipeline        shadowPipeline,
    VkPipelineLayout  shadowPipelineLayout,
    VkBuffer          vertexBuffer,
    VkBuffer          indexBuffer,
    uint32_t          indexCount,
    VkDescriptorSet   shadowDescriptorSet,
    const ShadowResources& shadowRes
) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin shadow command buffer!");
    }

    VkClearValue clearDepth{};
    clearDepth.depthStencil = { 1.f, 0 };

    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = shadowRenderPass;
    rpBegin.framebuffer = shadowFramebuffer;
    rpBegin.renderArea.offset = { 0, 0 };
    rpBegin.renderArea.extent = shadowExtent;
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearDepth;

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    // Bind shadow pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

    // Flip Y in the viewport
    VkViewport vp{};
    vp.x = 0.f;
    vp.y = (float)shadowExtent.height;
    vp.width = (float)shadowExtent.width;
    vp.height = -(float)shadowExtent.height; // negative to flip
    vp.minDepth = 0.f;
    vp.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = shadowExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Depth bias
    vkCmdSetDepthBias(cmd, 1.25f, 0.f, 1.75f);

    // Bind geometry
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    // Bind the single set=0 for shadow
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shadowPipelineLayout,
        0, /* firstSet=0 */
        1, &shadowDescriptorSet,
        0, nullptr
    );

    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(cmd);

    // Transition depth to SHADER_READ_ONLY_OPTIMAL
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = shadowRes.depthImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end shadow command buffer!");
    }
}


int main()
{
    GLFWwindow* window = nullptr;
    try {
        // Init window & callbacks
        if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(800, 600, "Vulkan Window", nullptr, nullptr);
        if (!window) throw std::runtime_error("Failed to create GLFW window!");

        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        // Hide cursor for FPS camera by default; toggle with CTRL
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // 1) Vulkan instance
        std::vector<const char*> extensions; // none required externally
        VulkanInstance vulkanInstance("My Vulkan App", VK_MAKE_VERSION(1, 0, 0), extensions);
        VkInstance instance = vulkanInstance.getInstance();

        // 2) Create a window surface
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }

        // 3) Physical + logical device
        PhysicalDevice physicalDevice(instance, surface);
        VkDevice device = physicalDevice.getDevice();

        // 4) SwapChain
        SwapChain swapChain(physicalDevice, device, surface, window);

        // 5) Main color & shadow RenderPass
        RenderPass renderPass(device,
            physicalDevice.getPhysicalDevice(),
            swapChain.getSwapChainImageFormat());

        // 6) The main GraphicsPipeline (2 sets: set=0=UBO, set=1=sampler)
        GraphicsPipeline graphicsPipeline(
            device,
            swapChain.getSwapChainExtent(),
            renderPass.getRenderPass()
        );
        swapChain.createFramebuffers(renderPass.getRenderPass());

        // 7) Command pool
        CommandPool commandPool(device, physicalDevice.getGraphicsQueueFamilyIndex());

        // ----------------------------------------------------------------------
        // Create geometry for the “cube”
        // ----------------------------------------------------------------------
        VkDeviceSize vbSize = sizeof(cubeVertices[0]) * cubeVertices.size();
        Buffer vertexBuffer(
            device,
            physicalDevice,
            vbSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        {
            void* data = nullptr;
            vkMapMemory(device, vertexBuffer.getMemory(), 0, vbSize, 0, &data);
            memcpy(data, cubeVertices.data(), (size_t)vbSize);
            vkUnmapMemory(device, vertexBuffer.getMemory());
        }

        VkDeviceSize ibSize = sizeof(cubeIndices[0]) * cubeIndices.size();
        Buffer indexBuffer(
            device,
            physicalDevice,
            ibSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        {
            void* data = nullptr;
            vkMapMemory(device, indexBuffer.getMemory(), 0, ibSize, 0, &data);
            memcpy(data, cubeIndices.data(), (size_t)ibSize);
            vkUnmapMemory(device, indexBuffer.getMemory());
        }

        // Cylinder geometry (light ray)
        const uint32_t cylinderSegments = 20;
        auto cylinderVertices = CylinderMesh::createCylinder(0.1f, 1.0f, cylinderSegments);
        auto cylinderIndices = CylinderMesh::createCylinderIndices(cylinderSegments);

        VkDeviceSize cylVbSize = sizeof(Vertex) * cylinderVertices.size();
        Buffer lightRayVertexBuffer(
            device,
            physicalDevice,
            cylVbSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        {
            void* data = nullptr;
            vkMapMemory(device, lightRayVertexBuffer.getMemory(), 0, cylVbSize, 0, &data);
            memcpy(data, cylinderVertices.data(), cylVbSize);
            vkUnmapMemory(device, lightRayVertexBuffer.getMemory());
        }

        VkDeviceSize cylIbSize = sizeof(uint16_t) * cylinderIndices.size();
        Buffer lightRayIndexBuffer(
            device,
            physicalDevice,
            cylIbSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        {
            void* data = nullptr;
            vkMapMemory(device, lightRayIndexBuffer.getMemory(), 0, cylIbSize, 0, &data);
            memcpy(data, cylinderIndices.data(), cylIbSize);
            vkUnmapMemory(device, lightRayIndexBuffer.getMemory());
        }

        // “menu plane” geometry
        VkDeviceSize planeVBSize = sizeof(Vertex) * planeVertices.size();
        Buffer planeVertexBuffer(
            device, physicalDevice, planeVBSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        {
            void* data = nullptr;
            vkMapMemory(device, planeVertexBuffer.getMemory(), 0, planeVBSize, 0, &data);
            memcpy(data, planeVertices.data(), planeVBSize);
            vkUnmapMemory(device, planeVertexBuffer.getMemory());
        }

        VkDeviceSize planeIBSize = sizeof(uint16_t) * planeIndices.size();
        Buffer planeIndexBuffer(
            device, physicalDevice, planeIBSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        {
            void* data = nullptr;
            vkMapMemory(device, planeIndexBuffer.getMemory(), 0, planeIBSize, 0, &data);
            memcpy(data, planeIndices.data(), planeIBSize);
            vkUnmapMemory(device, planeIndexBuffer.getMemory());
        }

        // ----------------------------------------------------------------------
        // UBO for main pass (set=0 => camera+model, set=1 => sampler)
        // ----------------------------------------------------------------------
        size_t swapCount = swapChain.getSwapChainImages().size();
        VkDeviceSize uboSize = sizeof(UniformBufferObject);

        std::vector<Buffer> uniformBuffers;
        uniformBuffers.reserve(swapCount);
        for (size_t i = 0; i < swapCount; i++) {
            uniformBuffers.emplace_back(
                device, physicalDevice, uboSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }

        // Light UBO: a small struct with a light matrix
        struct LightData {
            glm::mat4 lightViewProj;
        };
        VkDeviceSize lightUboSize = sizeof(LightData);

        std::vector<Buffer> lightBuffers;
        lightBuffers.reserve(swapCount);
        for (size_t i = 0; i < swapCount; i++) {
            lightBuffers.emplace_back(
                device, physicalDevice, lightUboSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }

        // Descriptor pool for those UBOs (set=0)
        VkDescriptorPool descriptorPoolUBO;
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            // enough for both “cube” UBO + “light” UBO across all swap images
            poolSize.descriptorCount = static_cast<uint32_t>(swapCount * 2);

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = static_cast<uint32_t>(swapCount * 2 + 10);

            if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPoolUBO) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create descriptor pool for UBO!");
            }
        }

        // Allocate the sets for the main pipeline (set=0)
        // We'll store them in descriptorSetsUBO.
        std::vector<VkDescriptorSet> descriptorSetsUBO(swapCount);
        {
            // we have a 2-set layout for the main pipeline: set=0=UBO, set=1=sampler
            // but here we only allocate set=0
            std::vector<VkDescriptorSetLayout> layouts(swapCount, graphicsPipeline.getDescriptorSetLayoutUBO());

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPoolUBO;
            allocInfo.descriptorSetCount = (uint32_t)swapCount;
            allocInfo.pSetLayouts = layouts.data();

            if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSetsUBO.data()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate descriptor sets (UBO)!");
            }

            for (size_t i = 0; i < swapCount; i++) {
                // binding=0 => camera+model UBO
                VkDescriptorBufferInfo camInfo{};
                camInfo.buffer = uniformBuffers[i].getBuffer();
                camInfo.offset = 0;
                camInfo.range = sizeof(UniformBufferObject);

                VkWriteDescriptorSet camWrite{};
                camWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                camWrite.dstSet = descriptorSetsUBO[i];
                camWrite.dstBinding = 0;
                camWrite.descriptorCount = 1;
                camWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                camWrite.pBufferInfo = &camInfo;

                // binding=1 => light UBO
                VkDescriptorBufferInfo lightInfo{};
                lightInfo.buffer = lightBuffers[i].getBuffer();
                lightInfo.offset = 0;
                lightInfo.range = sizeof(LightData);

                VkWriteDescriptorSet lightWrite{};
                lightWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                lightWrite.dstSet = descriptorSetsUBO[i];
                lightWrite.dstBinding = 1;
                lightWrite.descriptorCount = 1;
                lightWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                lightWrite.pBufferInfo = &lightInfo;

                std::array<VkWriteDescriptorSet, 2> writes = { camWrite, lightWrite };
                vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
            }
        }

        // The second set in the main pipeline (set=1) is for sampler
        // We'll have a combined image sampler for the compute output & one for the shadow map
        VkDescriptorPool descriptorPoolSampler;
        VkDescriptorSet  descriptorSetSampler; // we'll bind it at pipeline set=1
        VkSampler        samplerCompute;
        VkSampler        samplerShadowMap;
        {
            // We need 2 combined image samplers: one for compute, one for shadow
            VkDescriptorPoolSize poolSize{};
            poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSize.descriptorCount = 2;

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = 1;

            if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPoolSampler) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create descriptor pool (sampler)!");
            }

            // Create two actual samplers for compute & shadow
            {
                VkSamplerCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                info.magFilter = VK_FILTER_NEAREST;
                info.minFilter = VK_FILTER_NEAREST;
                info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                info.unnormalizedCoordinates = VK_FALSE;
                if (vkCreateSampler(device, &info, nullptr, &samplerCompute) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create sampler for compute image!");
                }
            }
            {
                VkSamplerCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                info.magFilter = VK_FILTER_NEAREST;
                info.minFilter = VK_FILTER_NEAREST;
                info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                info.unnormalizedCoordinates = VK_FALSE;
                if (vkCreateSampler(device, &info, nullptr, &samplerShadowMap) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create sampler for shadow map!");
                }
            }

            // Allocate set=1 from the main pipeline's "sampler" set layout
            VkDescriptorSetLayout samplerLayout = graphicsPipeline.getDescriptorSetLayoutSampler();
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPoolSampler;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &samplerLayout;

            if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetSampler) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate set=1 sampler descriptor set!");
            }
        }

        // Create the compute pass (PixelTracer) and set=1 => binding=0 for its image
        PixelTracer pixelTracer;
        pixelTracer.create(device, physicalDevice, 512, 512);
        {
            VkDescriptorImageInfo imgInfo{};
            imgInfo.sampler = samplerCompute;
            imgInfo.imageView = pixelTracer.getOutputImageView();
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = descriptorSetSampler; // same set=1
            write.dstBinding = 0; // binding=0 => compute image
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imgInfo;

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }

        // Shadow pipeline (only 1 set => the “light UBO”)
        ShadowPipeline shadowPipeline;
        shadowPipeline.create(device,
            renderPass.getShadowRenderPass(),
            graphicsPipeline.getDescriptorSetLayoutUBO());

        // Shadow resources: 2k x 2k
        ShadowResources shadowRes;
        {
            shadowRes.extent = { 2048, 2048 };

            VkImageCreateInfo imgInfo{};
            imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imgInfo.imageType = VK_IMAGE_TYPE_2D;
            imgInfo.extent = { shadowRes.extent.width, shadowRes.extent.height, 1 };
            imgInfo.mipLevels = 1;
            imgInfo.arrayLayers = 1;
            imgInfo.format = VK_FORMAT_D32_SFLOAT;
            imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (vkCreateImage(device, &imgInfo, nullptr, &shadowRes.depthImage) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shadow depth image!");
            }

            VkMemoryRequirements memReq;
            vkGetImageMemoryRequirements(device, shadowRes.depthImage, &memReq);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReq.size;
            allocInfo.memoryTypeIndex = physicalDevice.findMemoryType(
                memReq.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            if (vkAllocateMemory(device, &allocInfo, nullptr, &shadowRes.depthMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate memory for shadow depth!");
            }
            vkBindImageMemory(device, shadowRes.depthImage, shadowRes.depthMemory, 0);

            // Depth view
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = shadowRes.depthImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_D32_SFLOAT;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device, &viewInfo, nullptr, &shadowRes.depthView) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shadow depth image view!");
            }

            // Framebuffer
            VkImageView attachments[] = { shadowRes.depthView };
            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = renderPass.getShadowRenderPass();
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments = attachments;
            fbInfo.width = shadowRes.extent.width;
            fbInfo.height = shadowRes.extent.height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(device, &fbInfo, nullptr, &shadowRes.framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shadow framebuffer!");
            }
        }

        // Update set=1 => binding=1 with the shadow map
        {
            VkDescriptorImageInfo shadowMapInfo{};
            shadowMapInfo.sampler = samplerShadowMap;
            shadowMapInfo.imageView = shadowRes.depthView;
            shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptorSetSampler;
            w.dstBinding = 1; // binding=1 => shadow map
            w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            w.descriptorCount = 1;
            w.pImageInfo = &shadowMapInfo;

            vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
        }

        // Also we need a “shadow descriptor set” for each swapchain image (the shadow pipeline only uses set=0 => light data)
        std::vector<VkDescriptorSet> shadowDescriptorSets(swapCount);
        {
            std::vector<VkDescriptorSetLayout> layouts(swapCount, graphicsPipeline.getDescriptorSetLayoutUBO());
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPoolUBO;
            allocInfo.descriptorSetCount = (uint32_t)swapCount;
            allocInfo.pSetLayouts = layouts.data();

            if (vkAllocateDescriptorSets(device, &allocInfo, shadowDescriptorSets.data()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate shadow descriptor sets!");
            }

            for (size_t i = 0; i < swapCount; i++) {
                VkDescriptorBufferInfo bufInfo{};
                bufInfo.buffer = lightBuffers[i].getBuffer();
                bufInfo.offset = 0;
                bufInfo.range = sizeof(LightData);

                VkWriteDescriptorSet writeUBO{};
                writeUBO.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeUBO.dstSet = shadowDescriptorSets[i];
                writeUBO.dstBinding = 0;
                writeUBO.descriptorCount = 1;
                writeUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeUBO.pBufferInfo = &bufInfo;

                vkUpdateDescriptorSets(device, 1, &writeUBO, 0, nullptr);
            }
        }

        // Create the shadow pass command buffers (one per swap image)
        CommandBuffer shadowCmdBuffers(device, commandPool.getCommandPool(), (uint32_t)swapCount);
        {
            auto& scb = shadowCmdBuffers.getCommandBuffers();
            for (size_t i = 0; i < scb.size(); i++) {
                recordShadowCommandBuffer(
                    scb[i],
                    renderPass.getShadowRenderPass(),
                    shadowRes.framebuffer,
                    shadowRes.extent,
                    shadowPipeline.getPipeline(),
                    shadowPipeline.getPipelineLayout(),
                    vertexBuffer.getBuffer(),
                    indexBuffer.getBuffer(),
                    (uint32_t)cubeIndices.size(),
                    shadowDescriptorSets[i],
                    shadowRes
                );
            }
        }

        // Compute pass command buffer
        CommandBuffer computeCmdBuffer(device, commandPool.getCommandPool(), 1);
        {
            VkCommandBuffer cb = computeCmdBuffer.getCommandBuffers()[0];

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            if (vkBeginCommandBuffer(cb, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin compute cmd buffer!");
            }

            // Transition compute image to GENERAL
            {
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = pixelTracer.getOutputImage();
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                vkCmdPipelineBarrier(
                    cb,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    0,
                    0, nullptr, 0, nullptr, 1, &barrier
                );
            }

            // Bind compute pipeline
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pixelTracer.getPipeline());
            VkDescriptorSet ds = pixelTracer.getDescriptorSet();
            vkCmdBindDescriptorSets(
                cb,
                VK_PIPELINE_BIND_POINT_COMPUTE,
                pixelTracer.getPipelineLayout(),
                0, 1, &ds,
                0, nullptr
            );

            // Dispatch
            vkCmdDispatch(cb, 512, 512, 1);

            // Transition to SHADER_READ_ONLY_OPTIMAL
            {
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = pixelTracer.getOutputImage();
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(
                    cb,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr, 0, nullptr, 1, &barrier
                );
            }

            if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
                throw std::runtime_error("Failed to end compute command buffer!");
            }
        }

        // LightRay pipeline (only 1 set => the cylinder’s UBO)
        VkDeviceSize cylinderUBOSize = sizeof(UniformBufferObject);
        Buffer lightRayUBOBuffer(
            device, physicalDevice, cylinderUBOSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        VkDescriptorSetLayoutBinding lrBinding{};
        lrBinding.binding = 0;
        lrBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lrBinding.descriptorCount = 1;
        lrBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo lrLayoutInfo{};
        lrLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        lrLayoutInfo.bindingCount = 1;
        lrLayoutInfo.pBindings = &lrBinding;

        VkDescriptorSetLayout lightRayDescLayout;
        if (vkCreateDescriptorSetLayout(device, &lrLayoutInfo, nullptr, &lightRayDescLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create light ray descriptor layout!");
        }

        // Pool for the single set
        VkDescriptorPoolSize lrPoolSize{};
        lrPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lrPoolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo lrPoolInfo{};
        lrPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        lrPoolInfo.poolSizeCount = 1;
        lrPoolInfo.pPoolSizes = &lrPoolSize;
        lrPoolInfo.maxSets = 1;

        VkDescriptorPool lightRayDescriptorPool;
        if (vkCreateDescriptorPool(device, &lrPoolInfo, nullptr, &lightRayDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create light ray descriptor pool!");
        }

        VkDescriptorSet lightRayDescriptorSet;
        {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = lightRayDescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &lightRayDescLayout;

            if (vkAllocateDescriptorSets(device, &allocInfo, &lightRayDescriptorSet) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate light ray descriptor set!");
            }

            // binding=0 => the cylinder's MVP
            VkDescriptorBufferInfo lrBufInfo{};
            lrBufInfo.buffer = lightRayUBOBuffer.getBuffer();
            lrBufInfo.offset = 0;
            lrBufInfo.range = cylinderUBOSize;

            VkWriteDescriptorSet wr{};
            wr.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            wr.dstSet = lightRayDescriptorSet;
            wr.dstBinding = 0;
            wr.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wr.descriptorCount = 1;
            wr.pBufferInfo = &lrBufInfo;

            vkUpdateDescriptorSets(device, 1, &wr, 0, nullptr);
        }

        // Create that pipeline
        LightRayPipeline lightRayPipeline;
        lightRayPipeline.create(device, renderPass.getRenderPass(), lightRayDescLayout);

        // ----------------------------------------------------------------------
        // Now record the main pass command buffers
        // ----------------------------------------------------------------------
        CommandBuffer mainCmdBuffers(device, commandPool.getCommandPool(), (uint32_t)swapCount);

        auto recordMainPass = [&](CommandBuffer& cbObj) {
            const auto& cbs = cbObj.getCommandBuffers();
            for (size_t i = 0; i < cbs.size(); i++) {
                VkCommandBuffer cmd = cbs[i];

                VkCommandBufferBeginInfo bi{};
                bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to begin main cmd buffer!");
                }

                VkRenderPassBeginInfo rpBegin{};
                rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rpBegin.renderPass = renderPass.getRenderPass();
                rpBegin.framebuffer = swapChain.getSwapChainFramebuffers()[i];
                rpBegin.renderArea.offset = { 0,0 };
                rpBegin.renderArea.extent = swapChain.getSwapChainExtent();

                std::array<VkClearValue, 2> clears{};
                clears[0].color = { {0.f, 0.f, 0.f, 1.f} };
                clears[1].depthStencil = { 1.f, 0 };
                rpBegin.clearValueCount = (uint32_t)clears.size();
                rpBegin.pClearValues = clears.data();

                vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

                // (A) Bind the main pipeline (2 sets)
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.getPipeline());

                VkViewport viewport{};
                viewport.x = 0.f;
                viewport.y = 0.f;
                viewport.width = (float)swapChain.getSwapChainExtent().width;
                viewport.height = (float)swapChain.getSwapChainExtent().height;
                viewport.minDepth = 0.f;
                viewport.maxDepth = 1.f;
                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = { 0,0 };
                scissor.extent = swapChain.getSwapChainExtent();
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                // (1) Bind "cube" geometry
                VkDeviceSize offz[] = { 0 };
                VkBuffer vb = vertexBuffer.getBuffer();
                vkCmdBindVertexBuffers(cmd, 0, 1, &vb, offz);

                vkCmdBindIndexBuffer(cmd, indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT16);

                // Bind descriptor sets => set=0 => descriptorSetsUBO[i], set=1 => descriptorSetSampler
                // Notice we do two calls or an array of 2 sets
                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline.getPipelineLayout(),
                    0, // firstSet=0
                    1, &descriptorSetsUBO[i],
                    0, nullptr
                );
                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline.getPipelineLayout(),
                    1, // firstSet=1
                    1, &descriptorSetSampler,
                    0, nullptr
                );

                // Draw the cube
                vkCmdDrawIndexed(cmd, (uint32_t)cubeIndices.size(), 1, 0, 0, 0);

                // (2) Bind the lightRay pipeline for the cylinder
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, lightRayPipeline.getPipeline());
                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                // Bind set=0 => lightRay UBO
                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    lightRayPipeline.getPipelineLayout(),
                    0, // firstSet=0
                    1, &lightRayDescriptorSet,
                    0, nullptr
                );

                VkDeviceSize cylOff[] = { 0 };
                VkBuffer lrBuf = lightRayVertexBuffer.getBuffer();
                vkCmdBindVertexBuffers(cmd, 0, 1, &lrBuf, cylOff);

                vkCmdBindIndexBuffer(cmd, lightRayIndexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT16);
                vkCmdDrawIndexed(cmd, (uint32_t)cylinderIndices.size(), 1, 0, 0, 0);

                // (3) Switch back to the main pipeline, draw the "plane"
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.getPipeline());
                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                // We can re-bind the same sets or rely on the previous binding for set=0, set=1
                // but safer to re-bind them
                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline.getPipelineLayout(),
                    0, 1, &descriptorSetsUBO[i],
                    0, nullptr
                );
                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline.getPipelineLayout(),
                    1, 1, &descriptorSetSampler,
                    0, nullptr
                );

                VkDeviceSize planeOff[] = { 0 };
                VkBuffer planeBuf = planeVertexBuffer.getBuffer();
                vkCmdBindVertexBuffers(cmd, 0, 1, &planeBuf, planeOff);

                vkCmdBindIndexBuffer(cmd, planeIndexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT16);
                vkCmdDrawIndexed(cmd, (uint32_t)planeIndices.size(), 1, 0, 0, 0);

                vkCmdEndRenderPass(cmd);

                if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to end main command buffer!");
                }
            }
            };
        recordMainPass(mainCmdBuffers);

        // Create semaphores & fence
        VkSemaphore imageAvailableSem, renderFinishedSem;
        VkFence     inFlightFence;
        {
            VkSemaphoreCreateInfo semInfo{};
            semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            if (vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSem) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSem) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create sync objects!");
            }
        }

        // Retrieve queues
        VkQueue graphicsQueue;
        vkGetDeviceQueue(device, physicalDevice.getGraphicsQueueFamilyIndex(), 0, &graphicsQueue);

        VkQueue presentQueue;
        vkGetDeviceQueue(device, physicalDevice.getPresentQueueFamilyIndex(), 0, &presentQueue);

        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto fpsStartTime = std::chrono::high_resolution_clock::now();
        int  frameCount = 0;

        // We "select" the cube by default
        g_objectIsSelected = true;

        // Main loop
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Toggle cursor if user is pressing LEFT CTRL
            bool ctrlDown = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);
            if (ctrlDown && !g_showMouseCursor) {
                g_showMouseCursor = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else if (!ctrlDown && g_showMouseCursor) {
                g_showMouseCursor = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            // Simple picking
            if (g_showMouseCursor && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                // Stub: we could do real picking. For now, always set the same object:
                g_objectIsSelected = true;
            }

            // Check minimize
            int wWin, hWin;
            glfwGetFramebufferSize(window, &wWin, &hWin);
            if (wWin == 0 || hWin == 0) {
                glfwWaitEvents();
                continue;
            }

            // Wait for the previous frame
            vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

            // Acquire next swapchain image
            uint32_t imageIndex;
            {
                VkResult res = vkAcquireNextImageKHR(device, swapChain.getSwapChain(), UINT64_MAX,
                    imageAvailableSem, VK_NULL_HANDLE, &imageIndex);
                if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || framebufferResized) {
                    // Recreate next loop
                    continue;
                }
                else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
                    throw std::runtime_error("Failed to acquire swap chain image!");
                }
            }

            // Submit shadow pass
            vkResetFences(device, 1, &inFlightFence);
            {
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                VkCommandBuffer scb = shadowCmdBuffers.getCommandBuffers()[imageIndex];
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &scb;
                if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to submit shadow pass!");
                }
                vkQueueWaitIdle(graphicsQueue);
            }

            // Submit compute pass
            {
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                VkCommandBuffer ccb = computeCmdBuffer.getCommandBuffers()[0];
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &ccb;
                if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to submit compute pass!");
                }
                vkQueueWaitIdle(graphicsQueue);
            }

            // Update transforms
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;

            // Basic camera movement
            if (!g_showMouseCursor) {
                float speed = g_cameraMoveSpeed * deltaTime;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    cameraPos += speed * cameraFront;
                }
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    cameraPos -= speed * cameraFront;
                }
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                    cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
                }
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
                }
            }

            // Move selected object with numeric keypad 4,6,8,2,7,9
            if (g_objectIsSelected) {
                float objSpeed = 4.f * deltaTime;
                if (glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS) {
                    g_selectedObjectPos.x -= objSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS) {
                    g_selectedObjectPos.x += objSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_KP_8) == GLFW_PRESS) {
                    g_selectedObjectPos.z -= objSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS) {
                    g_selectedObjectPos.z += objSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_KP_7) == GLFW_PRESS) {
                    g_selectedObjectPos.y -= objSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_KP_9) == GLFW_PRESS) {
                    g_selectedObjectPos.y += objSpeed;
                }
            }

            // 4) Update the “cube” UBO (model/view/proj)
            {
                UniformBufferObject ubo{};
                ubo.model = glm::translate(glm::mat4(1.f), g_selectedObjectPos);
                ubo.view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
                ubo.proj = glm::perspective(glm::radians(45.f),
                    swapChain.getSwapChainExtent().width /
                    (float)swapChain.getSwapChainExtent().height,
                    0.1f, 100.f);
                // flip Y
                ubo.proj[1][1] *= -1.f;

                void* dataPtr = nullptr;
                vkMapMemory(device, uniformBuffers[imageIndex].getMemory(), 0, sizeof(ubo), 0, &dataPtr);
                memcpy(dataPtr, &ubo, sizeof(ubo));
                vkUnmapMemory(device, uniformBuffers[imageIndex].getMemory());
            }

            // 5) Update Light UBO
            {
                LightData lData{};
                glm::vec3 lightPos(5.f, 10.f, 5.f);
                glm::vec3 lookAt(0.f, 0.f, 0.f);
                glm::vec3 up(0.f, 1.f, 0.f);
                glm::mat4 lightView = glm::lookAt(lightPos, lookAt, up);
                glm::mat4 lightProj = glm::perspective(glm::radians(45.f), 1.f, 0.1f, 100.f);
                lData.lightViewProj = lightProj * lightView;

                void* dataPtr = nullptr;
                vkMapMemory(device, lightBuffers[imageIndex].getMemory(), 0, sizeof(lData), 0, &dataPtr);
                memcpy(dataPtr, &lData, sizeof(lData));
                vkUnmapMemory(device, lightBuffers[imageIndex].getMemory());
            }

            // 6) Update the “light ray” UBO
            {
                // A “light->target” cylinder
                glm::vec3  lightPos(5.f, 10.f, 5.f);
                glm::vec3  target(0.f, 0.f, 0.f);
                glm::vec3  dir = glm::normalize(target - lightPos);
                float      length = glm::length(target - lightPos);

                glm::vec3 upVec(0.f, 1.f, 0.f);
                glm::mat4 rot(1.f);
                if (glm::length(glm::cross(upVec, dir)) > 0.0001f) {
                    glm::vec3 axis = glm::normalize(glm::cross(upVec, dir));
                    float angle = acos(glm::dot(upVec, dir));
                    rot = glm::rotate(glm::mat4(1.f), angle, axis);
                }
                glm::mat4 modelRay =
                    glm::translate(glm::mat4(1.f), lightPos) *
                    rot *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.f, length, 1.f));

                glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
                glm::mat4 proj = glm::perspective(glm::radians(45.f),
                    swapChain.getSwapChainExtent().width /
                    (float)swapChain.getSwapChainExtent().height,
                    0.1f, 100.f);
                proj[1][1] *= -1.f;

                UniformBufferObject lrUBO{};
                lrUBO.model = modelRay;
                lrUBO.view = view;
                lrUBO.proj = proj;

                void* dataPtr = nullptr;
                vkMapMemory(device, lightRayUBOBuffer.getMemory(), 0, sizeof(lrUBO), 0, &dataPtr);
                memcpy(dataPtr, &lrUBO, sizeof(lrUBO));
                vkUnmapMemory(device, lightRayUBOBuffer.getMemory());
            }

            // 7) Submit the main pass
            {
                vkResetFences(device, 1, &inFlightFence);

                // We'll wait on imageAvailableSem
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                VkSemaphore waitSems[] = { imageAvailableSem };
                VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSems;
                submitInfo.pWaitDstStageMask = waitStages;

                VkCommandBuffer mainCB = mainCmdBuffers.getCommandBuffers()[imageIndex];
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &mainCB;

                VkSemaphore signalSems[] = { renderFinishedSem };
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSems;

                if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to submit main pass!");
                }

                // Present
                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSems;
                VkSwapchainKHR swapChains[] = { swapChain.getSwapChain() };
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapChains;
                presentInfo.pImageIndices = &imageIndex;

                VkResult res = vkQueuePresentKHR(presentQueue, &presentInfo);
                if (res == VK_ERROR_OUT_OF_DATE_KHR ||
                    res == VK_SUBOPTIMAL_KHR ||
                    framebufferResized)
                {
                    // Recreate next loop
                }
                else if (res != VK_SUCCESS) {
                    throw std::runtime_error("Failed to present swap chain image!");
                }
            }

            // FPS
            frameCount++;
            auto fpsNow = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<float>(fpsNow - fpsStartTime).count() >= 1.f) {
                float fps = frameCount / std::chrono::duration<float>(fpsNow - fpsStartTime).count();
                std::cout << "FPS: " << fps << std::endl;
                fpsStartTime = fpsNow;
                frameCount = 0;
            }
        }

        vkDeviceWaitIdle(device);
        std::cout << "Device idle. Cleaning up...\n";

        // Destroy plane geometry
        planeVertexBuffer.destroy();
        planeIndexBuffer.destroy();

        // Destroy cylinder
        lightRayPipeline.destroy(device);
        vkDestroyDescriptorPool(device, lightRayDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, lightRayDescLayout, nullptr);
        lightRayUBOBuffer.destroy();
        lightRayVertexBuffer.destroy();
        lightRayIndexBuffer.destroy();

        // Shadow pipeline
        shadowPipeline.destroy(device);

        // Compute
        pixelTracer.destroy(device);

        // Samplers
        vkDestroySampler(device, samplerCompute, nullptr);
        vkDestroySampler(device, samplerShadowMap, nullptr);
        vkDestroyDescriptorPool(device, descriptorPoolSampler, nullptr);

        // UBO
        for (auto& ub : uniformBuffers) {
            ub.destroy();
        }
        for (auto& lb : lightBuffers) {
            lb.destroy();
        }
        vkDestroyDescriptorPool(device, descriptorPoolUBO, nullptr);

        // Shadow
        vkDestroyFramebuffer(device, shadowRes.framebuffer, nullptr);
        vkDestroyImageView(device, shadowRes.depthView, nullptr);
        vkDestroyImage(device, shadowRes.depthImage, nullptr);
        vkFreeMemory(device, shadowRes.depthMemory, nullptr);

        // Buffers for the cube
        vertexBuffer.destroy();
        indexBuffer.destroy();

        // Swapchain
        swapChain.destroy();

        // Render pass
        renderPass.destroy(device);

        // Graphics pipeline
        graphicsPipeline.destroy(device);

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "Cleanup done.\n";
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
