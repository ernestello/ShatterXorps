// ShadowPipeline.h
#ifndef SHADOW_PIPELINE_H
#define SHADOW_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class ShadowPipeline {
public:
    ShadowPipeline() = default;
    ~ShadowPipeline() = default;

    void create(VkDevice device, VkRenderPass shadowPass, VkDescriptorSetLayout uboLayout);
    void destroy(VkDevice device);

    VkPipeline       getPipeline()       const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

private:
    VkPipeline       pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);
};

#endif // SHADOW_PIPELINE_H
