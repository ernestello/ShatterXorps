// GraphicsPipeline.h
#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

// Initialization: Define GraphicsPipeline class | GraphicsPipeline.h | Used by main.cpp and other classes | Encapsulates Vulkan graphics pipeline functionality | Class Definition - To manage pipeline resources | Depends on Vulkan device, SwapChain, RenderPass | High computing power | Defined once at [line 4 - GraphicsPipeline.h - global scope] | GPU
class GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device, VkExtent2D swapChainExtent, VkRenderPass renderPass);
    ~GraphicsPipeline();

    VkPipeline getPipeline() const { return graphicsPipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

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
