#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
#include <optional>
#include <set>
#include "SwapChainSupportDetails.h" // Include the new header

class PhysicalDevice {
public:
    PhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    ~PhysicalDevice();

    VkDevice getDevice() const { return logicalDevice; };
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkPhysicalDeviceProperties getProperties() const { return deviceProperties; }

    uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex; }
    uint32_t getPresentQueueFamilyIndex() const { return presentQueueFamilyIndex; }

    SwapChainSupportDetails querySwapChainSupport(); // Updated signature
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkSurfaceKHR getSurface() const { return surface; } // Added accessor

private:
    VkDevice logicalDevice;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties deviceProperties = {};

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;

    VkSurfaceKHR surface; // Added member

    void pickPhysicalDevice(VkInstance instance);
    void createLogicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    void findQueueFamilies(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};

#endif // PHYSICAL_DEVICE_H
