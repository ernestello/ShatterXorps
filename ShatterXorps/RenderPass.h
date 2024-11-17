// RenderPass.h
#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>

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
