// RenderPass.h
#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>

class RenderPass {
public:
    RenderPass(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapChainImageFormat, bool enableDepth = true);
    ~RenderPass();

    // The *main* color pass
    VkRenderPass getRenderPass() const { return renderPass; }

    // The new *shadow* pass
    VkRenderPass getShadowRenderPass() const { return shadowRenderPass; }

    // Destroy
    void destroy(VkDevice device);

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkRenderPass renderPass;      // main color pass
    VkRenderPass shadowRenderPass; // new shadow pass

    bool depthAttachment;

    void createRenderPass(VkFormat swapChainImageFormat, bool enableDepth);
    void createShadowRenderPass(); // new

    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
};

#endif // RENDER_PASS_H
