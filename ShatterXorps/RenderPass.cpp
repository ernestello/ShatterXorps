// RenderPass.cpp
#include "RenderPass.h"
#include <array>
#include <iostream>

// Constructor implementation
RenderPass::RenderPass(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat, bool enableDepth)
    : device(device), physicalDevice(physicalDevice), renderPass(VK_NULL_HANDLE), depthAttachment(enableDepth) {
    createRenderPass(swapChainImageFormat, enableDepth);
}

// Destructor implementation
RenderPass::~RenderPass() {
    // Destructor intentionally left empty.
    // Destruction is handled explicitly via the destroy() method.
}

// Destroy function
void RenderPass::destroy(VkDevice device) {
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
}

// Creates the render pass with optional depth attachment
void RenderPass::createRenderPass(VkFormat swapChainImageFormat, bool enableDepth) {
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

    std::vector<VkAttachmentDescription> attachments;
    attachments.emplace_back(colorAttachment);

    // Color attachment reference
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // Attachment index in the attachments array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment
    VkAttachmentReference depthAttachmentRef{};
    if (enableDepth) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.emplace_back(depthAttachment);

        depthAttachmentRef.attachment = 1; // Depth attachment is second in the attachments array
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    if (enableDepth) {
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    // Subpass dependencies
    std::vector<VkSubpassDependency> dependencies;

    // Dependency 1
    VkSubpassDependency dependency1{};
    dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency1.dstSubpass = 0;
    dependency1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency1.srcAccessMask = 0;
    dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.emplace_back(dependency1);

    if (enableDepth) {
        // Dependency 2
        VkSubpassDependency dependency2{};
        dependency2.srcSubpass = 0;
        dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency2.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency2.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies.emplace_back(dependency2);
    }

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

// Finds a suitable depth format
VkFormat RenderPass::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

// Finds a supported format from candidates
VkFormat RenderPass::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features) {
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

    throw std::runtime_error("Failed to find supported format!");
}
