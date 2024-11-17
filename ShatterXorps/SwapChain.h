// SwapChain.h
#ifndef SWAP_CHAIN_H
#define SWAP_CHAIN_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <optional>

#include "PhysicalDevice.h"
#include "SwapChainSupportDetails.h"

// Manages Vulkan swap chain and related resources
class SwapChain {
public:
    // Constructor: Initializes swap chain, image views, and depth resources
    SwapChain(PhysicalDevice& physicalDevice, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window);

    // Destructor: Cleans up all swap chain resources
    ~SwapChain();

    // Delete copy constructor and copy assignment operator
    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    // Move constructor and move assignment operator
    SwapChain(SwapChain&& other) noexcept;
    SwapChain& operator=(SwapChain&& other) noexcept;

    // Accessor functions
    VkSwapchainKHR getSwapChain() const { return swapChain; }
    const std::vector<VkImage>& getSwapChainImages() const { return swapChainImages; }
    VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
    const std::vector<VkImageView>& getSwapChainImageViews() const { return swapChainImageViews; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapChainFramebuffers; }
    VkSurfaceKHR getSurface() const { return surface; }

    // Destroys swap chain and associated resources
    void destroy();

    // Creates framebuffers for each swap chain image
    void createFramebuffers(VkRenderPass renderPass);

private:
    PhysicalDevice* physicalDevice; // Pointer to the physical device
    VkDevice device;                // Logical Vulkan device
    VkSurfaceKHR surface;           // Vulkan surface
    VkSwapchainKHR swapChain;       // Swap chain handle
    VkFormat swapChainImageFormat;  // Format of swap chain images
    VkExtent2D swapChainExtent;     // Dimensions of swap chain images
    std::vector<VkImage> swapChainImages;               // Swap chain images
    std::vector<VkImageView> swapChainImageViews;       // Image views for swap chain images
    std::vector<VkFramebuffer> swapChainFramebuffers;    // Framebuffers for each swap chain image

    // Depth resources
    VkImage depthImage;                  // Depth image
    VkDeviceMemory depthImageMemory;     // Memory for depth image
    VkImageView depthImageView;          // Image view for depth image

    // Internal helper functions
    void createSwapChain(GLFWwindow* window);
    void createImageViews();
    void createDepthResources();
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
        VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
        VkFormatFeatureFlags features);

    // Selection helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
};

#endif // SWAP_CHAIN_H
