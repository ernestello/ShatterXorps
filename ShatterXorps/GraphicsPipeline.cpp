// GraphicsPipeline.cpp
#include "GraphicsPipeline.h"
#include "Vertex.h"
#include <fstream>
#include <stdexcept>
#include <array>
#include <vector>
#include <iostream>

GraphicsPipeline::GraphicsPipeline(
    VkDevice device,
    VkExtent2D swapChainExtent,
    VkRenderPass renderPass)
    : device(device),
    pipelineLayout(VK_NULL_HANDLE),
    graphicsPipeline(VK_NULL_HANDLE),
    descriptorSetLayoutUBO(VK_NULL_HANDLE),
    descriptorSetLayoutSampler(VK_NULL_HANDLE)
{
    createGraphicsPipeline(swapChainExtent, renderPass);
}

GraphicsPipeline::~GraphicsPipeline() {
    // Actual cleanup is in destroy()
}

void GraphicsPipeline::destroy(VkDevice device) {
    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    if (descriptorSetLayoutSampler != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutSampler, nullptr);
        descriptorSetLayoutSampler = VK_NULL_HANDLE;
    }
    if (descriptorSetLayoutUBO != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutUBO, nullptr);
        descriptorSetLayoutUBO = VK_NULL_HANDLE;
    }
}

void GraphicsPipeline::createGraphicsPipeline(VkExtent2D swapChainExtent, VkRenderPass renderPass) {
    //-----------------------------------------------------------------
    // 1) Load SPIR-V vertex & fragment shaders
    //-----------------------------------------------------------------
    auto vertShaderCode = readFile("shaders/shader.vert.spv");
    auto fragShaderCode = readFile("shaders/shader.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};

    // Vertex stage
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";

    // Fragment stage
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";

    //-----------------------------------------------------------------
    // 2) Vertex Input
    //-----------------------------------------------------------------
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    //-----------------------------------------------------------------
    // 3) Input Assembly
    //-----------------------------------------------------------------
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    //-----------------------------------------------------------------
    // 4) Viewport & Scissor (Dynamic)
    //-----------------------------------------------------------------
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    //-----------------------------------------------------------------
    // 5) Rasterizer
    //-----------------------------------------------------------------
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // or VK_CULL_MODE_BACK_BIT
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    //-----------------------------------------------------------------
    // 6) Multisampling
    //-----------------------------------------------------------------
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //-----------------------------------------------------------------
    // 7) Depth & Stencil
    //-----------------------------------------------------------------
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    //-----------------------------------------------------------------
    // 8) Color Blending
    //-----------------------------------------------------------------
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    //-----------------------------------------------------------------
    // 9) Dynamic States
    //-----------------------------------------------------------------
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    //-----------------------------------------------------------------
    // 10) Descriptor Set Layouts
    //
    //     We have 2 sets:
    //
    //     set=0 -> UBO layout:
    //        binding=0 => camera MVP
    //        binding=1 => light MVP
    //
    //     set=1 -> Sampler layout:
    //        binding=0 => PixelTracer (or any additional sampler)
    //        binding=1 => Shadow map sampler
    //-----------------------------------------------------------------

    // (A) set=0 (UBO with camera + light)
    VkDescriptorSetLayoutBinding camUBOBinding{};
    camUBOBinding.binding = 0;
    camUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    camUBOBinding.descriptorCount = 1;
    camUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // e.g. camera MVP in vertex
    camUBOBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding lightUBOBinding{};
    lightUBOBinding.binding = 1;
    lightUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightUBOBinding.descriptorCount = 1;
    // We might read light data in vertex & fragment
    lightUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    lightUBOBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> uboBindings = {
        camUBOBinding,
        lightUBOBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfoUBO{};
    layoutInfoUBO.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfoUBO.bindingCount = static_cast<uint32_t>(uboBindings.size());
    layoutInfoUBO.pBindings = uboBindings.data();

    if (vkCreateDescriptorSetLayout(
        device, &layoutInfoUBO, nullptr, &descriptorSetLayoutUBO) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout (UBO)!");
    }

    // (B) set=1 (Samplers: PixelTracer at binding=0, ShadowMap at binding=1)
    //  - binding=0 => existing PixelTracer image
    //  - binding=1 => new shadow depth sampler
    VkDescriptorSetLayoutBinding samplerBinding0{};
    samplerBinding0.binding = 0;
    samplerBinding0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding0.descriptorCount = 1;
    samplerBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding0.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerBinding1{};
    samplerBinding1.binding = 1;
    samplerBinding1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding1.descriptorCount = 1;
    samplerBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding1.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> samplerBindings = {
        samplerBinding0,
        samplerBinding1
    };

    VkDescriptorSetLayoutCreateInfo layoutInfoSampler{};
    layoutInfoSampler.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfoSampler.bindingCount = static_cast<uint32_t>(samplerBindings.size());
    layoutInfoSampler.pBindings = samplerBindings.data();

    if (vkCreateDescriptorSetLayout(
        device, &layoutInfoSampler, nullptr, &descriptorSetLayoutSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout (samplers)!");
    }

    // Combine both sets (set=0 for UBO, set=1 for samplers)
    std::array<VkDescriptorSetLayout, 2> setLayouts = {
        descriptorSetLayoutUBO,
        descriptorSetLayoutSampler
    };

    //-----------------------------------------------------------------
    // 11) Pipeline Layout
    //-----------------------------------------------------------------
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;  // if needed, add push constants

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    //-----------------------------------------------------------------
    // 12) Create the final Graphics Pipeline
    //-----------------------------------------------------------------
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // Clean up shader modules after pipeline creation
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    return shaderModule;
}

std::vector<char> GraphicsPipeline::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}
