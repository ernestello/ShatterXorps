#ifndef LIGHT_RAY_PIPELINE_H
#define LIGHT_RAY_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

// This pipeline will render the light–ray (cylinder) geometry.
// It expects a descriptor set layout (for a UBO with an MVP matrix)
class LightRayPipeline {
public:
    LightRayPipeline() = default;
    ~LightRayPipeline() = default;

    // Create the pipeline given the device, the render pass (main pass),
    // and a descriptor set layout (for binding a uniform buffer).
    void create(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout lightRayDescriptorSetLayout);
    void destroy(VkDevice device);

    VkPipeline getPipeline() const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

private:
    VkPipeline       pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
};

#endif // LIGHT_RAY_PIPELINE_H
