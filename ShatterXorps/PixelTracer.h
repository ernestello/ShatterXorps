#pragma once

#include <vulkan/vulkan.h>

// Forward-declare if you have a "PhysicalDevice" class
class PhysicalDevice;

/*
  PixelTracer encapsulates a compute pipeline that writes ray-traced output
  into a storage image (outputImage). Now also includes minimal uniform buffers
  for the "camera" and "scene" so we can match the shader's set=0, binding=0..2.
*/
class PixelTracer
{
public:
    PixelTracer();
    ~PixelTracer();

    // Create all resources for the compute pass
    void create(VkDevice device, PhysicalDevice& physDevice, uint32_t width, uint32_t height);

    // Destroy the compute resources
    void destroy(VkDevice device);

    // Accessors
    VkPipeline       getPipeline() const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VkDescriptorSet  getDescriptorSet() const { return descriptorSet; }

    VkImage     getOutputImage() const { return outputImage; }
    VkImageView getOutputImageView() const { return outputImageView; }

private:
    // The compute pipeline
    VkPipeline       pipeline;
    VkPipelineLayout pipelineLayout;
    VkShaderModule   shaderModule;

    // Descriptor layout/pool/set
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool      descriptorPool;
    VkDescriptorSet       descriptorSet;

    // The image we write our compute output to
    VkImage        outputImage;
    VkDeviceMemory outputMemory;
    VkImageView    outputImageView;

    // Minimal uniform buffers for "camera" and "scene"
    VkBuffer       cameraBuffer;
    VkDeviceMemory cameraBufferMemory;
    VkBuffer       sceneBuffer;
    VkDeviceMemory sceneBufferMemory;

    uint32_t width;
    uint32_t height;
};
