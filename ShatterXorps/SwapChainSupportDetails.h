// SwapChainSupportDetails.h
#ifndef SWAPCHAIN_SUPPORT_DETAILS_H
#define SWAPCHAIN_SUPPORT_DETAILS_H

#include <vulkan/vulkan.h>
#include <vector>

// Initialization: Define SwapChainSupportDetails struct | SwapChainSupportDetails.h | Used by PhysicalDevice.cpp and SwapChain.cpp | Holds swap chain support information | Struct Definition - To store swap chain capabilities | Depends on Vulkan surface support | Minimal computing power | Defined once at [line 4 - SwapChainSupportDetails.h - global scope] | GPU
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

#endif // SWAPCHAIN_SUPPORT_DETAILS_H
