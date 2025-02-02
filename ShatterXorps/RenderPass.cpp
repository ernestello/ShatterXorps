// RenderPass.cpp
#include "RenderPass.h"
#include <array>
#include <iostream>

/* ------------------------------------------------------------------------------------
 * Constructor: we still create the “regular” color pass here, plus we create a
 * separate shadow pass if desired.
 * ------------------------------------------------------------------------------------ */
RenderPass::RenderPass(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat, bool enableDepth)
    : device(device)
    , physicalDevice(physicalDevice)
    , renderPass(VK_NULL_HANDLE)
    , depthAttachment(enableDepth)
    , shadowRenderPass(VK_NULL_HANDLE)  // <-- ADD for shadow pass
{
    // 1) Create the main (color) render pass
    createRenderPass(swapChainImageFormat, enableDepth);

    // 2) Create a separate "shadow" render pass for depth-only
    createShadowRenderPass();
}

RenderPass::~RenderPass() {
    // destruction is done via destroy()
}

void RenderPass::destroy(VkDevice device) {
    // Destroy the main render pass
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
    // Destroy the shadow render pass
    if (shadowRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, shadowRenderPass, nullptr);
        shadowRenderPass = VK_NULL_HANDLE;
    }
}

/* ------------------------------------------------------------------------------------
 * This is your existing color pass creation. It has 1 color attachment, optional depth.
 * No changes except we keep it as “main” pass.
 * ------------------------------------------------------------------------------------ */
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

    // Color attachment ref
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment (optional)
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

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    if (enableDepth) {
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    // Dependencies
    std::vector<VkSubpassDependency> dependencies;

    // Basic external->subpass
    VkSubpassDependency dep1{};
    dep1.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep1.dstSubpass = 0;
    dep1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep1.srcAccessMask = 0;
    dep1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.push_back(dep1);

    if (enableDepth) {
        // Basic subpass->external for depth
        VkSubpassDependency dep2{};
        dep2.srcSubpass = 0;
        dep2.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep2.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dep2.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dep2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies.push_back(dep2);
    }

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpInfo.pAttachments = attachments.data();
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    rpInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    rpInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create main render pass!");
    }
    std::cout << "Main (color) render pass created successfully.\n";
}

/* ------------------------------------------------------------------------------------
 *  NEW FUNCTION:
 *  Create a *shadow pass* that is depth-only. Usually you want a depth format that
 *  you can read from as a sampler for your shadows.
 * ------------------------------------------------------------------------------------ */
void RenderPass::createShadowRenderPass()
{
    // Single depth attachment, no color attachments
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(); // e.g. D32_SFLOAT
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // We want to keep the depth
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // finalLayout depends how you want to read it. If you want to sample as read-only:
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0; // no color
    subpass.pColorAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depthRef;

    // Dependencies to handle layout transitions
    std::array<VkSubpassDependency, 2> deps{};

    // EXTERNAL -> SUBPASS
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // SUBPASS -> EXTERNAL
    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    // Create the render pass
    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &depthAttachment;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    rpInfo.dependencyCount = static_cast<uint32_t>(deps.size());
    rpInfo.pDependencies = deps.data();

    if (vkCreateRenderPass(device, &rpInfo, nullptr, &shadowRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow render pass!");
    }
    std::cout << "Shadow (depth-only) render pass created successfully.\n";
}

VkFormat RenderPass::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

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
    throw std::runtime_error("Failed to find supported depth format!");
}
