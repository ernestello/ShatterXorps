#ifndef FRUSTUM_STATIC_PIPELINE_H
#define FRUSTUM_STATIC_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class FrustumStaticPipeline {
public:
    FrustumStaticPipeline() = default;
    ~FrustumStaticPipeline() = default;

    // Create and destroy
    void create(VkDevice device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout cameraSetLayout);

    void destroy(VkDevice device);

    // Accessors
    VkPipeline       getPipeline()       const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

private:
    VkPipeline       pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    // Private helpers that must be in the .cpp:
    std::vector<char>  readFile(const std::string& filename);
    VkShaderModule     createShaderModule(VkDevice device, const std::vector<char>& code);
};

#endif // FRUSTUM_STATIC_PIPELINE_H
