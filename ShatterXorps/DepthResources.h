#pragma once
#ifndef DEPTH_RESOURCES_H
#define DEPTH_RESOURCES_H

#include <vulkan/vulkan.h>
#include "PhysicalDevice.h"

class DepthResources {
public:
    DepthResources() = default;
    ~DepthResources() = default;

    // Create the depth image, memory, and view
    void create(
        VkDevice device,
        PhysicalDevice& physicalDevice,
        uint32_t width,
        uint32_t height
    );

    // Destroy all depth-related resources
    void destroy(VkDevice device);

    // Getters
    VkImage        getImage()      const { return depthImage; }
    VkImageView    getImageView()  const { return depthImageView; }
    VkDeviceMemory getMemory()     const { return depthImageMemory; }
    VkFormat       getDepthFormat() const { return depthFormat; }

private:
    VkImage        depthImage = VK_NULL_HANDLE;
    VkImageView    depthImageView = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkFormat       depthFormat = VK_FORMAT_UNDEFINED;

    // Helper to check if format has a stencil component
    bool hasStencilComponent(VkFormat format) const;

    // Helper to find the correct depth format
    VkFormat findDepthFormat(const PhysicalDevice& physicalDevice);
    VkFormat findSupportedFormat(
        const PhysicalDevice& physicalDevice,
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    );

    // Helper for creating the actual image
    void createImage(
        VkDevice device,
        PhysicalDevice& physicalDevice,
        uint32_t width,
        uint32_t height
    );
};

#endif // DEPTH_RESOURCES_H
