#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device, VkExtent2D swapChainExtent, VkRenderPass renderPass);
    ~GraphicsPipeline();

    void destroy(VkDevice device);

    VkPipeline getPipeline() const { return graphicsPipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

    // set=0 -> UBO layout (camera + light)
    VkDescriptorSetLayout getDescriptorSetLayoutUBO() const { return descriptorSetLayoutUBO; }

    // set=1 -> Sampler layout (PixelTracer at binding=0, ShadowMap at binding=1)
    VkDescriptorSetLayout getDescriptorSetLayoutSampler() const { return descriptorSetLayoutSampler; }

private:
    VkDevice device;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    // set=0 layout
    VkDescriptorSetLayout descriptorSetLayoutUBO;

    // set=1 layout
    VkDescriptorSetLayout descriptorSetLayoutSampler;

    void createGraphicsPipeline(VkExtent2D swapChainExtent, VkRenderPass renderPass);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);
};

#endif // GRAPHICS_PIPELINE_H
