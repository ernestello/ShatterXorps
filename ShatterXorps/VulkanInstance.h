// VulkanInstance.h
#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

// Initialization: Define VulkanInstance class | VulkanInstance.h | Used by main.cpp and other classes | Encapsulates Vulkan instance functionality | Class Definition - To manage Vulkan instance and debug messenger | Depends on Vulkan instance and extensions | Minimal to moderate computing power | Defined once at [line 4 - VulkanInstance.h - global scope] | CPU, GPU
class VulkanInstance {
public:
    VulkanInstance(const std::string& appName, uint32_t appVersion, const std::vector<const char*>& extensions);
    ~VulkanInstance();

    VkInstance getInstance() const { return instance; }

private:
    VkInstance instance;
    VkApplicationInfo appInfo;
    VkInstanceCreateInfo createInfo;
    VkDebugUtilsMessengerEXT debugMessenger;

    std::vector<const char*> requiredExtensions;
    std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

    bool checkValidationLayerSupport();

    void setupDebugMessenger();
    void createAppInfo(const std::string& appName, uint32_t appVersion);
    void createInstance();

    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT              messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    // Helper functions to load extension functions
    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);
};

#endif // VULKAN_INSTANCE_H
