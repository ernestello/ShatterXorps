// RenderPass.cpp

#include "RenderPass.h"
#include <array>
#include <iostream>

// Initialization: Define RenderPass constructor | RenderPass.cpp | Used by main.cpp - line where RenderPass is instantiated | Creates render pass with color and depth attachments | Constructor - To configure rendering process | Depends on Vulkan device, PhysicalDevice, SwapChain | Moderate computing power | Once at [line 6 - RenderPass.cpp - constructor] | GPU
RenderPass::RenderPass(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat)
    : device(device), physicalDevice(physicalDevice), renderPass(VK_NULL_HANDLE) {
    createRenderPass(swapChainImageFormat);
}

// Initialization: Define RenderPass destructor | RenderPass.cpp | Used by main.cpp - line where RenderPass is destroyed | Destroys render pass | Destructor - To release render pass resources | Depends on Vulkan device memory | Minimal computing power | Once at [line 12 - RenderPass.cpp - destructor] | GPU
RenderPass::~RenderPass() {
    // Ensure resources are cleaned up
    destroy(device); // Alternatively, call destroy here
}

// Initialization: Define render pass destruction function | RenderPass.cpp | Used by destructor and swap chain recreation | Destroys Vulkan render pass | Resource Cleanup - To release GPU resources | Depends on Vulkan device memory | Minimal computing power | Once at [line 17 - RenderPass.cpp - destroy] | GPU
void RenderPass::destroy(VkDevice device) {
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
}

// Initialization: Define render pass creation function | RenderPass.cpp | Used by RenderPass constructor | Configures color and depth attachments, subpasses, and dependencies | Render Pass Configuration - To define rendering steps | Depends on swap chain format and physical device capabilities | Moderate computing power | Once at [line 23 - RenderPass.cpp - createRenderPass] | GPU
void RenderPass::createRenderPass(VkFormat swapChainImageFormat) {
    // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Attachment references
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // Attachment index in the attachments array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1; // Depth attachment is second in the attachments array
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Subpass dependencies
    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    // Attachments array
    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    // Render pass create info
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    // Create render pass
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }

    std::cout << "Render pass created successfully." << std::endl;
}

// Initialization: Define depth format finding function | RenderPass.cpp | Used by createRenderPass | Selects appropriate depth format | Depth Format Identification - To support depth testing | Depends on physical device's supported formats | Minimal computing power | Once at [line 88 - RenderPass.cpp - findDepthFormat] | GPU
VkFormat RenderPass::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

// Initialization: Define supported format finding function | RenderPass.cpp | Used by findDepthFormat | Checks available formats for required features | Supported Format Selection - To ensure depth attachment compatibility | Depends on physical device's format properties | Minimal computing power | Once at [line 93 - RenderPass.cpp - findSupportedFormat] | GPU
VkFormat RenderPass::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a supported format!");
}
