// GraphicsPipeline.h
#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

// Forward declaration
struct Vertex;

class GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device, VkExtent2D swapChainExtent, VkRenderPass renderPass);
    ~GraphicsPipeline();

    VkPipeline getPipeline() const { return graphicsPipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    // Destroys the graphics pipeline and related resources
    void destroy(VkDevice device);

private:
    VkDevice device;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkDescriptorSetLayout descriptorSetLayout;

    void createGraphicsPipeline(VkExtent2D swapChainExtent, VkRenderPass renderPass);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);
};

#endif // GRAPHICS_PIPELINE_H
