// PhysicalDevice.cpp

#include "PhysicalDevice.h"
#include <stdexcept>
#include <set>
#include <cstring>

// Initialization: Define PhysicalDevice constructor | PhysicalDevice.cpp | Used by main.cpp - line where PhysicalDevice is instantiated | Selects and initializes physical and logical devices | Constructor - To set up GPU and logical device | Depends on Vulkan instance and surface | Moderate computing power | Once at [line 6 - PhysicalDevice.cpp - constructor] | GPU
PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
    : surface(surface) {
    pickPhysicalDevice(instance);
    createLogicalDevice();
}

// Initialization: Define PhysicalDevice destructor | PhysicalDevice.cpp | Used by main.cpp - line where PhysicalDevice is destroyed | Destroys logical device | Destructor - To release device resources | Depends on Vulkan device memory | Minimal computing power | Once at [line 12 - PhysicalDevice.cpp - destructor] | GPU
PhysicalDevice::~PhysicalDevice() {
    if (logicalDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(logicalDevice, nullptr);
    }
}

// Initialization: Define physical device selection function | PhysicalDevice.cpp | Used by constructor | Chooses a suitable physical device | Device Selection - To utilize GPU capabilities | Depends on Vulkan instance and surface | Moderate computing power | Once at [line 17 - PhysicalDevice.cpp - pickPhysicalDevice] | GPU
void PhysicalDevice::pickPhysicalDevice(VkInstance instance) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

// Initialization: Define device suitability check function | PhysicalDevice.cpp | Used by pickPhysicalDevice | Determines if a device meets required criteria | Suitability Check - To ensure device supports necessary features | Depends on device capabilities and extensions | Minimal computing power | Once per device at [line 32 - PhysicalDevice.cpp - isDeviceSuitable] | GPU
bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device) {
    findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return graphicsQueueFamilyIndex != UINT32_MAX &&
        presentQueueFamilyIndex != UINT32_MAX &&
        extensionsSupported && swapChainAdequate;
}

// Initialization: Define queue family finding function | PhysicalDevice.cpp | Used by isDeviceSuitable | Identifies queue families for graphics and presentation | Queue Family Identification - To utilize GPU queues | Depends on device's queue families | Minimal computing power | Once per device at [line 44 - PhysicalDevice.cpp - findQueueFamilies] | GPU
void PhysicalDevice::findQueueFamilies(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int index = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyIndex = index;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);

        if (presentSupport) {
            presentQueueFamilyIndex = index;
        }

        if (graphicsQueueFamilyIndex != UINT32_MAX && presentQueueFamilyIndex != UINT32_MAX) {
            break;
        }

        index++;
    }
}

// Initialization: Define device extension support check function | PhysicalDevice.cpp | Used by isDeviceSuitable | Checks if device supports required extensions | Extension Support Check - To ensure device can handle swap chains | Depends on device's available extensions | Minimal computing power | Once per device at [line 66 - PhysicalDevice.cpp - checkDeviceExtensionSupport] | GPU
bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

// Initialization: Define logical device creation function | PhysicalDevice.cpp | Used by constructor | Creates Vulkan logical device with required queue families | Logical Device Setup - To interface with physical device | Depends on queue families and device extensions | Moderate computing power | Once at [line 80 - PhysicalDevice.cpp - createLogicalDevice] | GPU
void PhysicalDevice::createLogicalDevice() {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }
}

// Initialization: Define swap chain support querying function | PhysicalDevice.cpp | Used by isDeviceSuitable | Retrieves swap chain support details | Swap Chain Support Query - To assess swap chain capabilities | Depends on device's surface support | Minimal computing power | Once per device at [line 105 - PhysicalDevice.cpp - querySwapChainSupport] | GPU
SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// Initialization: Define memory type finding function | PhysicalDevice.cpp | Used by createBuffer and SwapChain.cpp | Identifies suitable memory types for buffers and images | Memory Type Identification - To allocate appropriate GPU memory | Depends on device's memory properties | Minimal computing power | Once per memory allocation at [line 126 - PhysicalDevice.cpp - findMemoryType] | GPU
uint32_t PhysicalDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}
