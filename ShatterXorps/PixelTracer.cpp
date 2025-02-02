#include "PixelTracer.h"
#include "PhysicalDevice.h"
#include <stdexcept>
#include <vector>
#include <fstream>
#include <cstring>   // for memcpy

static std::vector<char> readFile(const char* filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open file: ") + filename);
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

// A small struct for camera data in compute
struct CameraDataGPU {
    // Must match layout in raytrace.comp
    float cameraPos[3];
    float _pad0;          // pad out to 16 bytes
    float invProjection[16]; // if you want to fill them
    float invView[16];
    float screenSize[2];
    float _pad1[2];
};

// A small struct for scene data in compute
struct SceneDataGPU {
    // Must match layout in raytrace.comp
    float lightDir[3];
    float _pad0;
    float lightColor[3];
    float _pad1;
};

PixelTracer::PixelTracer()
    : pipeline(VK_NULL_HANDLE),
    pipelineLayout(VK_NULL_HANDLE),
    shaderModule(VK_NULL_HANDLE),
    descriptorSetLayout(VK_NULL_HANDLE),
    descriptorPool(VK_NULL_HANDLE),
    descriptorSet(VK_NULL_HANDLE),
    outputImage(VK_NULL_HANDLE),
    outputMemory(VK_NULL_HANDLE),
    outputImageView(VK_NULL_HANDLE),
    cameraBuffer(VK_NULL_HANDLE),
    cameraBufferMemory(VK_NULL_HANDLE),
    sceneBuffer(VK_NULL_HANDLE),
    sceneBufferMemory(VK_NULL_HANDLE),
    width(0),
    height(0)
{
}

PixelTracer::~PixelTracer()
{
    // Normally call destroy() explicitly before destructor if needed
}

void PixelTracer::create(VkDevice device, PhysicalDevice& physDevice, uint32_t w, uint32_t h)
{
    width = w;
    height = h;

    //------------------------------------------------------
    // 0) Create minimal UBOs for camera & scene
    //    so validation passes. Real app might pass them in.
    //------------------------------------------------------
    {
        VkDeviceSize camSize = sizeof(CameraDataGPU);
        VkDeviceSize sceneSize = sizeof(SceneDataGPU);

        // Create cameraBuffer
        {
            VkBufferCreateInfo bci{};
            bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bci.size = camSize;
            bci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device, &bci, nullptr, &cameraBuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create PixelTracer camera buffer!");
            }
            VkMemoryRequirements memReq;
            vkGetBufferMemoryRequirements(device, cameraBuffer, &memReq);

            VkMemoryAllocateInfo alloc{};
            alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc.allocationSize = memReq.size;
            alloc.memoryTypeIndex = physDevice.findMemoryType(
                memReq.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            if (vkAllocateMemory(device, &alloc, nullptr, &cameraBufferMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate PixelTracer camera buffer memory!");
            }
            vkBindBufferMemory(device, cameraBuffer, cameraBufferMemory, 0);

            // Fill it with some dummy data
            CameraDataGPU dummy{};
            dummy.cameraPos[0] = 0.f;
            dummy.cameraPos[1] = 0.f;
            dummy.cameraPos[2] = 0.f;
            dummy.screenSize[0] = (float)width;
            dummy.screenSize[1] = (float)height;

            void* dataPtr = nullptr;
            vkMapMemory(device, cameraBufferMemory, 0, camSize, 0, &dataPtr);
            std::memcpy(dataPtr, &dummy, sizeof(dummy));
            vkUnmapMemory(device, cameraBufferMemory);
        }
        // Create sceneBuffer
        {
            VkBufferCreateInfo bci{};
            bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bci.size = sceneSize;
            bci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device, &bci, nullptr, &sceneBuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create PixelTracer scene buffer!");
            }
            VkMemoryRequirements memReq;
            vkGetBufferMemoryRequirements(device, sceneBuffer, &memReq);

            VkMemoryAllocateInfo alloc{};
            alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc.allocationSize = memReq.size;
            alloc.memoryTypeIndex = physDevice.findMemoryType(
                memReq.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            if (vkAllocateMemory(device, &alloc, nullptr, &sceneBufferMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate PixelTracer scene buffer memory!");
            }
            vkBindBufferMemory(device, sceneBuffer, sceneBufferMemory, 0);

            // Fill with dummy data
            SceneDataGPU dummy{};
            dummy.lightDir[0] = 0.f;   // e.g. overhead
            dummy.lightDir[1] = -1.f;
            dummy.lightDir[2] = 0.f;
            dummy.lightColor[0] = 1.f; // white
            dummy.lightColor[1] = 1.f;
            dummy.lightColor[2] = 1.f;

            void* dataPtr = nullptr;
            vkMapMemory(device, sceneBufferMemory, 0, sceneSize, 0, &dataPtr);
            std::memcpy(dataPtr, &dummy, sizeof(dummy));
            vkUnmapMemory(device, sceneBufferMemory);
        }
    }

    //------------------------------------------------------
    // 1) Create the output storage image + view
    //------------------------------------------------------
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        imageInfo.extent = { width, height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device, &imageInfo, nullptr, &outputImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create PixelTracer output image!");
        }

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, outputImage, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex =
            physDevice.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &outputMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate PixelTracer image memory!");
        }
        vkBindImageMemory(device, outputImage, outputMemory, 0);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = outputImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &outputImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create PixelTracer image view!");
        }
    }

    //------------------------------------------------------
    // 2) Descriptor set layout: (0) camera UBO, (1) scene UBO, (2) storage image
    //------------------------------------------------------
    {
        VkDescriptorSetLayoutBinding bindingCamera{};
        bindingCamera.binding = 0;
        bindingCamera.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindingCamera.descriptorCount = 1;
        bindingCamera.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding bindingScene{};
        bindingScene.binding = 1;
        bindingScene.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindingScene.descriptorCount = 1;
        bindingScene.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding bindingImage{};
        bindingImage.binding = 2;
        bindingImage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindingImage.descriptorCount = 1;
        bindingImage.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            bindingCamera, bindingScene, bindingImage
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = (uint32_t)bindings.size();
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create PixelTracer descriptor set layout!");
        }
    }

    //------------------------------------------------------
    // 3) Pipeline layout
    //------------------------------------------------------
    {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create PixelTracer pipeline layout!");
        }
    }

    //------------------------------------------------------
    // 4) Load the compute shader ("raytrace.comp.spv")
    //------------------------------------------------------
    {
        auto spirv = readFile("shaders/raytrace.comp.spv");
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = spirv.size();
        moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(spirv.data());

        if (vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create PixelTracer shader module!");
        }
    }

    //------------------------------------------------------
    // 5) Create the compute pipeline
    //------------------------------------------------------
    {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = shaderModule;
        stageInfo.pName = "main";

        VkComputePipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeInfo.stage = stageInfo;
        pipeInfo.layout = pipelineLayout;

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create PixelTracer compute pipeline!");
        }
    }

    //------------------------------------------------------
    // 6) Descriptor pool + single set
    //------------------------------------------------------
    {
        // We need 2 UBO descriptors + 1 storage image
        std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  1 }
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create PixelTracer descriptor pool!");
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate PixelTracer descriptor set!");
        }

        // (A) Camera UBO => binding=0
        VkDescriptorBufferInfo camInfo{};
        camInfo.buffer = cameraBuffer;
        camInfo.offset = 0;
        camInfo.range = sizeof(CameraDataGPU);

        VkWriteDescriptorSet writeCam{};
        writeCam.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeCam.dstSet = descriptorSet;
        writeCam.dstBinding = 0;
        writeCam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeCam.descriptorCount = 1;
        writeCam.pBufferInfo = &camInfo;

        // (B) Scene UBO => binding=1
        VkDescriptorBufferInfo sceneInfo{};
        sceneInfo.buffer = sceneBuffer;
        sceneInfo.offset = 0;
        sceneInfo.range = sizeof(SceneDataGPU);

        VkWriteDescriptorSet writeScene{};
        writeScene.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeScene.dstSet = descriptorSet;
        writeScene.dstBinding = 1;
        writeScene.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeScene.descriptorCount = 1;
        writeScene.pBufferInfo = &sceneInfo;

        // (C) Storage image => binding=2
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageView = outputImageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        // The actual sampler is optional for storage images

        VkWriteDescriptorSet writeImg{};
        writeImg.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeImg.dstSet = descriptorSet;
        writeImg.dstBinding = 2;
        writeImg.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeImg.descriptorCount = 1;
        writeImg.pImageInfo = &imageInfo;

        std::vector<VkWriteDescriptorSet> writes = { writeCam, writeScene, writeImg };
        vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
    }
}

void PixelTracer::destroy(VkDevice device)
{
    // Destroy pipeline
    if (pipeline) {
        vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    // Destroy pipeline layout
    if (pipelineLayout) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    // Destroy shader module
    if (shaderModule) {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        shaderModule = VK_NULL_HANDLE;
    }
    // Destroy descriptor pool
    if (descriptorPool) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    // Destroy descriptor set layout
    if (descriptorSetLayout) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    // Destroy the uniform buffers
    if (cameraBuffer) {
        vkDestroyBuffer(device, cameraBuffer, nullptr);
        cameraBuffer = VK_NULL_HANDLE;
    }
    if (cameraBufferMemory) {
        vkFreeMemory(device, cameraBufferMemory, nullptr);
        cameraBufferMemory = VK_NULL_HANDLE;
    }
    if (sceneBuffer) {
        vkDestroyBuffer(device, sceneBuffer, nullptr);
        sceneBuffer = VK_NULL_HANDLE;
    }
    if (sceneBufferMemory) {
        vkFreeMemory(device, sceneBufferMemory, nullptr);
        sceneBufferMemory = VK_NULL_HANDLE;
    }
    // Destroy image view
    if (outputImageView) {
        vkDestroyImageView(device, outputImageView, nullptr);
        outputImageView = VK_NULL_HANDLE;
    }
    // Destroy image
    if (outputImage) {
        vkDestroyImage(device, outputImage, nullptr);
        outputImage = VK_NULL_HANDLE;
    }
    // Free memory
    if (outputMemory) {
        vkFreeMemory(device, outputMemory, nullptr);
        outputMemory = VK_NULL_HANDLE;
    }
}
