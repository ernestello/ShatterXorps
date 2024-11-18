// RenderPass.h
#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>

// RenderPass class encapsulates Vulkan render pass functionality
class RenderPass {
public:
    // Constructor: Adds a parameter to enable/disable depth attachment
    RenderPass(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat, bool enableDepth = true);
    ~RenderPass();

    VkRenderPass getRenderPass() const { return renderPass; }

    // Destroys the render pass
    void destroy(VkDevice device);

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkRenderPass renderPass;

    bool depthAttachment; // Flag indicating if depth is enabled

    // Creates the render pass with optional depth
    void createRenderPass(VkFormat swapChainImageFormat, bool enableDepth);

    // Finds a suitable depth format
    VkFormat findDepthFormat();

    // Finds a supported format from candidates
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
};

#endif // RENDER_PASS_H
