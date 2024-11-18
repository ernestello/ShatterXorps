// SwapChainSupportDetails.h
#ifndef SWAPCHAIN_SUPPORT_DETAILS_H
#define SWAPCHAIN_SUPPORT_DETAILS_H

#include <vulkan/vulkan.h>
#include <vector>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

#endif // SWAPCHAIN_SUPPORT_DETAILS_H
