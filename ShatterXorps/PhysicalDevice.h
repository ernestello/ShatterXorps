// PhysicalDevice.h
#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include <vulkan/vulkan.h>
#include "SwapChainSupportDetails.h"
#include <vector>

// Initialization: Define PhysicalDevice class | PhysicalDevice.h | Used by main.cpp and other classes | Encapsulates Vulkan physical and logical device functionality | Class Definition - To manage device resources | Depends on Vulkan instance and surface | Moderate computing power | Defined once at [line 4 - PhysicalDevice.h - global scope] | GPU
class PhysicalDevice {
public:
    PhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    ~PhysicalDevice();

    VkDevice getDevice() const { return logicalDevice; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkPhysicalDeviceProperties getProperties() const { return deviceProperties; }

    uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex; }
    uint32_t getPresentQueueFamilyIndex() const { return presentQueueFamilyIndex; }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    // Delete copy constructor and copy assignment operator
    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;

private:
    VkDevice logicalDevice;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties deviceProperties{};

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;

    VkSurfaceKHR surface;

    void pickPhysicalDevice(VkInstance instance);
    void createLogicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    void findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};

#endif // PHYSICAL_DEVICE_H
