#include "DepthResources.h"
#include <stdexcept>
#include <iostream> // for debug prints

// Helper: does the chosen depth format have a stencil component?
bool DepthResources::hasStencilComponent(VkFormat format) const {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// Public: create
void DepthResources::create(
    VkDevice device,
    PhysicalDevice& physicalDevice,
    uint32_t width,
    uint32_t height
) {
    // 1. Find a suitable depth format
    depthFormat = findDepthFormat(physicalDevice);

    // [DEBUG PRINT] – let's see which format was picked
    std::cout << "[DepthResources::create] Chose depthFormat = " << depthFormat << std::endl;

    // 2. Create the image, memory, and image view
    createImage(device, physicalDevice, width, height);

    // [Optional debug print] – see the resulting image view
    std::cout << "[DepthResources::create] Created DepthImageView = " << depthImageView << std::endl;
}

// Public: destroy
void DepthResources::destroy(VkDevice device) {
    // If already destroyed, do nothing
    if (depthImageView == VK_NULL_HANDLE && depthImage == VK_NULL_HANDLE && depthImageMemory == VK_NULL_HANDLE) {
        // Already destroyed
        return;
    }

    std::cout << "[Debug][DepthResources::destroy] device=" << device
        << " destroying depthImageView=" << depthImageView
        << ", depthImage=" << depthImage
        << ", depthImageMemory=" << depthImageMemory << std::endl;

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
}
// Private: findDepthFormat
VkFormat DepthResources::findDepthFormat(const PhysicalDevice& physicalDevice) {
    return findSupportedFormat(
        physicalDevice,
        {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

// Private: findSupportedFormat
VkFormat DepthResources::findSupportedFormat(
    const PhysicalDevice& physicalDevice,
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features
) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice.getPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
            (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a supported depth format!");
}

// Private: createImage
void DepthResources::createImage(
    VkDevice device,
    PhysicalDevice& physicalDevice,
    uint32_t width,
    uint32_t height
) {
    // 1) Create a VkImage for depth
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image!");
    }

    // 2) Allocate memory
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, depthImage, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = physicalDevice.findMemoryType(
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate depth image memory!");
    }

    // 3) Bind memory
    vkBindImageMemory(device, depthImage, depthImageMemory, 0);

    // 4) Create an Image View for the depth image
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencilComponent(depthFormat)) {
        // If format includes stencil, we must also include that aspect bit
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image view!");
    }
    std::cout << "[Debug][DepthResources::createImage] device=" << device
        << " Created depthImageView=" << depthImageView
        << ", depthFormat=" << depthFormat
        << std::endl;
}
