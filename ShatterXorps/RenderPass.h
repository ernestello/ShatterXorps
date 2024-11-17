// RenderPass.h
#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>

// Initialization: Define RenderPass class | RenderPass.h | Used by main.cpp and other classes | Encapsulates Vulkan render pass functionality | Class Definition - To manage render pass resources | Depends on Vulkan device, PhysicalDevice, SwapChain | Moderate computing power | Defined once at [line 4 - RenderPass.h - global scope] | GPU
class RenderPass {
public:
    RenderPass(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat);
    ~RenderPass();

    VkRenderPass getRenderPass() const { return renderPass; }

    void destroy(VkDevice device); // **Added destroy method**

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkRenderPass renderPass;

    void createRenderPass(VkFormat swapChainImageFormat);
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
};

#endif // RENDER_PASS_H
