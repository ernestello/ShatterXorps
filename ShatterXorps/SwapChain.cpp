// SwapChain.cpp
#include "SwapChain.h"
#include "RenderPass.h" // Ensure RenderPass is included
#include <array>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <iostream>

// SwapChain constructor
SwapChain::SwapChain(PhysicalDevice& physicalDeviceRef, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window)
    : physicalDevice(&physicalDeviceRef), device(device), surface(surface), swapChain(VK_NULL_HANDLE),
    offscreenImage(VK_NULL_HANDLE), offscreenImageMemory(VK_NULL_HANDLE), offscreenImageView(VK_NULL_HANDLE),
    offscreenFramebuffer(VK_NULL_HANDLE), offscreenRenderPass(nullptr) {
    createSwapChain(window);
    createImageViews();
    createDepthResources();
    createOffscreenResources();       // Initialize offscreen resources
    createOffscreenFramebuffer();    // Create framebuffer for offscreen rendering
}

// SwapChain move constructor
SwapChain::SwapChain(SwapChain&& other) noexcept
    : physicalDevice(other.physicalDevice),
    device(other.device),
    surface(other.surface),
    swapChain(other.swapChain),
    swapChainImageFormat(other.swapChainImageFormat),
    swapChainExtent(other.swapChainExtent),
    swapChainImages(std::move(other.swapChainImages)),
    swapChainImageViews(std::move(other.swapChainImageViews)),
    swapChainFramebuffers(std::move(other.swapChainFramebuffers)),
    depthImage(other.depthImage),
    depthImageMemory(other.depthImageMemory),
    depthImageView(other.depthImageView),
    offscreenImage(other.offscreenImage),
    offscreenImageMemory(other.offscreenImageMemory),
    offscreenImageView(other.offscreenImageView),
    offscreenFramebuffer(other.offscreenFramebuffer),
    offscreenRenderPass(other.offscreenRenderPass) {
    other.swapChain = VK_NULL_HANDLE;
    other.depthImage = VK_NULL_HANDLE;
    other.depthImageMemory = VK_NULL_HANDLE;
    other.depthImageView = VK_NULL_HANDLE;
    other.offscreenImage = VK_NULL_HANDLE;
    other.offscreenImageMemory = VK_NULL_HANDLE;
    other.offscreenImageView = VK_NULL_HANDLE;
    other.offscreenFramebuffer = VK_NULL_HANDLE;
    other.offscreenRenderPass = nullptr;
}

// SwapChain move assignment operator
SwapChain& SwapChain::operator=(SwapChain&& other) noexcept {
    if (this != &other) {
        destroy();

        physicalDevice = other.physicalDevice;
        device = other.device;
        surface = other.surface;
        swapChain = other.swapChain;
        swapChainImageFormat = other.swapChainImageFormat;
        swapChainExtent = other.swapChainExtent;
        swapChainImages = std::move(other.swapChainImages);
        swapChainImageViews = std::move(other.swapChainImageViews);
        swapChainFramebuffers = std::move(other.swapChainFramebuffers);
        depthImage = other.depthImage;
        depthImageMemory = other.depthImageMemory;
        depthImageView = other.depthImageView;
        offscreenImage = other.offscreenImage;
        offscreenImageMemory = other.offscreenImageMemory;
        offscreenImageView = other.offscreenImageView;
        offscreenFramebuffer = other.offscreenFramebuffer;
        offscreenRenderPass = other.offscreenRenderPass;

        other.swapChain = VK_NULL_HANDLE;
        other.depthImage = VK_NULL_HANDLE;
        other.depthImageMemory = VK_NULL_HANDLE;
        other.depthImageView = VK_NULL_HANDLE;
        other.offscreenImage = VK_NULL_HANDLE;
        other.offscreenImageMemory = VK_NULL_HANDLE;
        other.offscreenImageView = VK_NULL_HANDLE;
        other.offscreenFramebuffer = VK_NULL_HANDLE;
        other.offscreenRenderPass = nullptr;
    }
    return *this;
}

// SwapChain destructor
SwapChain::~SwapChain() {
    destroy();
}

// SwapChain destroy function
void SwapChain::destroy() {
    // Destroy offscreen framebuffer and render pass
    if (offscreenFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, offscreenFramebuffer, nullptr);
        offscreenFramebuffer = VK_NULL_HANDLE;
    }

    if (offscreenRenderPass != nullptr) {
        offscreenRenderPass->destroy(device);
        delete offscreenRenderPass;
        offscreenRenderPass = nullptr;
    }

    // Destroy offscreen image view and image
    if (offscreenImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, offscreenImageView, nullptr);
        offscreenImageView = VK_NULL_HANDLE;
    }

    if (offscreenImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, offscreenImage, nullptr);
        offscreenImage = VK_NULL_HANDLE;
    }

    if (offscreenImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, offscreenImageMemory, nullptr);
        offscreenImageMemory = VK_NULL_HANDLE;
    }

    // Destroy depth resources
    if (depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthImageView, nullptr);
        depthImageView = VK_NULL_HANDLE;
    }

    if (depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, depthImage, nullptr);
        depthImage = VK_NULL_HANDLE;
    }

    if (depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, depthImageMemory, nullptr);
        depthImageMemory = VK_NULL_HANDLE;
    }

    // Destroy framebuffers
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    swapChainFramebuffers.clear();

    // Destroy image views
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    swapChainImageViews.clear();

    // Destroy swap chain
    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }

    // Clear swap chain images
    swapChainImages.clear();
}

// SwapChain createSwapChain function
void SwapChain::createSwapChain(GLFWwindow* window) {
    SwapChainSupportDetails swapChainSupport = physicalDevice->querySwapChainSupport(physicalDevice->getPhysicalDevice());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {
        physicalDevice->getGraphicsQueueFamilyIndex(),
        physicalDevice->getPresentQueueFamilyIndex()
    };

    if (physicalDevice->getGraphicsQueueFamilyIndex() != physicalDevice->getPresentQueueFamilyIndex()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create Vulkan swap chain
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

// SwapChain createImageViews function
void SwapChain::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void SwapChain::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        physicalDevice->findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

// SwapChain createDepthResources function
void SwapChain::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    // Assuming PhysicalDevice has a method to find memory type
    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory
    );

    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// SwapChain findDepthFormat function
VkFormat SwapChain::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

// SwapChain findSupportedFormat function
VkFormat SwapChain::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice->getPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

// SwapChain chooseSwapSurfaceFormat function
VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

// SwapChain chooseSwapPresentMode function
VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

// SwapChain chooseSwapExtent function
VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

// SwapChain createImageView function
VkImageView SwapChain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }

    return imageView;
}

// SwapChain createFramebuffers function
void SwapChain::createFramebuffers(VkRenderPass renderPass) {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

// SwapChain createOffscreenResources function
void SwapChain::createOffscreenResources() {
    // Define the size of the thumbnail
    uint32_t thumbnailWidth = 200;
    uint32_t thumbnailHeight = 150;

    // Create the offscreen render pass without depth
    offscreenRenderPass = new RenderPass(device, physicalDevice->getPhysicalDevice(), swapChainImageFormat, false);

    // Create the offscreen image
    createImage(
        thumbnailWidth,
        thumbnailHeight,
        swapChainImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offscreenImage,
        offscreenImageMemory
    );

    // Create the image view for the offscreen image
    offscreenImageView = createImageView(offscreenImage, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

// SwapChain createOffscreenFramebuffer function
void SwapChain::createOffscreenFramebuffer() {
    if (offscreenRenderPass == nullptr || offscreenImageView == VK_NULL_HANDLE) {
        throw std::runtime_error("Offscreen render pass or image view not initialized!");
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = offscreenRenderPass->getRenderPass();
    framebufferInfo.attachmentCount = 1; // Only color attachment
    framebufferInfo.pAttachments = &offscreenImageView;
    framebufferInfo.width = 200;  // Thumbnail width
    framebufferInfo.height = 150; // Thumbnail height
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &offscreenFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen framebuffer!");
    }
}
