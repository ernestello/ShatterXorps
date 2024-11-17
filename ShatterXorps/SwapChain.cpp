// SwapChain.cpp
#include "SwapChain.h"
#include <array>
#include <algorithm>
#include <stdexcept>
#include <limits>

// Initialization: Define SwapChain constructor | SwapChain.cpp | Used by main.cpp - during swap chain creation | Initializes swap chain with physical device, logical device, surface, and window | Constructor - To set up swap chain resources | Depends on PhysicalDevice and Vulkan device | Moderate computing power | Once per swap chain creation at [line 6 - SwapChain.cpp - constructor] | GPU
SwapChain::SwapChain(PhysicalDevice& physicalDeviceRef, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window)
    : physicalDevice(&physicalDeviceRef), device(device), surface(surface), swapChain(VK_NULL_HANDLE) {
    createSwapChain(window);
    createImageViews();
    createDepthResources();
}

// Initialization: Define SwapChain move constructor | SwapChain.cpp | Used by main.cpp - during swap chain relocation | Moves resources from another SwapChain instance to this one | Move Constructor - To transfer ownership of resources | Depends on PhysicalDevice and Vulkan device | Minimal computing power | Once per swap chain relocation at [line 16 - SwapChain.cpp - move constructor] | GPU
SwapChain::SwapChain(SwapChain&& other) noexcept
    : physicalDevice(other.physicalDevice),
    device(other.device),
    surface(other.surface),
    swapChain(other.swapChain),
    swapChainImageFormat(other.swapChainImageFormat),
    swapChainExtent(other.swapChainExtent),
    swapChainImages(std::move(other.swapChainImages)),
    swapChainImageViews(std::move(other.swapChainImageViews)),
    swapChainFramebuffers(std::move(other.swapChainFramebuffers)),
    depthImage(other.depthImage),
    depthImageMemory(other.depthImageMemory),
    depthImageView(other.depthImageView) {
    other.swapChain = VK_NULL_HANDLE;
    other.depthImage = VK_NULL_HANDLE;
    other.depthImageMemory = VK_NULL_HANDLE;
    other.depthImageView = VK_NULL_HANDLE;
}

// Initialization: Define SwapChain move assignment operator | SwapChain.cpp | Used by main.cpp - during swap chain relocation | Assigns resources from another SwapChain instance to this one | Move Assignment - To transfer ownership of resources | Depends on PhysicalDevice and Vulkan device | Minimal computing power | Once per swap chain relocation at [line 32 - SwapChain.cpp - move assignment] | GPU
SwapChain& SwapChain::operator=(SwapChain&& other) noexcept {
    if (this != &other) {
        destroy();

        physicalDevice = other.physicalDevice;
        device = other.device;
        surface = other.surface;
        swapChain = other.swapChain;
        swapChainImageFormat = other.swapChainImageFormat;
        swapChainExtent = other.swapChainExtent;
        swapChainImages = std::move(other.swapChainImages);
        swapChainImageViews = std::move(other.swapChainImageViews);
        swapChainFramebuffers = std::move(other.swapChainFramebuffers);
        depthImage = other.depthImage;
        depthImageMemory = other.depthImageMemory;
        depthImageView = other.depthImageView;

        other.swapChain = VK_NULL_HANDLE;
        other.depthImage = VK_NULL_HANDLE;
        other.depthImageMemory = VK_NULL_HANDLE;
        other.depthImageView = VK_NULL_HANDLE;
    }
    return *this;
}

// Initialization: Define SwapChain destructor | SwapChain.cpp | Used by main.cpp - during SwapChain object destruction | Destroys swap chain and related resources | Destructor - To clean up swap chain resources | Depends on Vulkan device memory | Minimal computing power | Once per swap chain destruction at [line 54 - SwapChain.cpp - destructor] | GPU
SwapChain::~SwapChain() {
    destroy();
}

// Cleanup: Define swap chain destruction function | SwapChain.cpp | Used by destructor and swap chain recreation | Destroys all swap chain related Vulkan resources | Resource Cleanup - To release GPU resources associated with swap chain | Depends on Vulkan device memory | Minimal computing power | Once per swap chain destruction at [line 60 - SwapChain.cpp - destroy] | GPU
void SwapChain::destroy() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    depthImageView = VK_NULL_HANDLE;
    depthImage = VK_NULL_HANDLE;
    depthImageMemory = VK_NULL_HANDLE;
    swapChainFramebuffers.clear();
    swapChainImageViews.clear();
    swapChainImages.clear();
    swapChain = VK_NULL_HANDLE;
}

// Initialization: Define swap chain creation function | SwapChain.cpp | Used by constructor and swap chain recreation | Creates Vulkan swap chain based on surface capabilities and window properties | Swap Chain Setup - To handle image buffers for rendering | Depends on PhysicalDevice, Vulkan device, and GLFW window | Moderate computing power | Once per swap chain creation at [line 82 - SwapChain.cpp - createSwapChain] | GPU
void SwapChain::createSwapChain(GLFWwindow* window) {
    SwapChainSupportDetails swapChainSupport = physicalDevice->querySwapChainSupport(physicalDevice->getPhysicalDevice());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {
        physicalDevice->getGraphicsQueueFamilyIndex(),
        physicalDevice->getPresentQueueFamilyIndex()
    };

    if (physicalDevice->getGraphicsQueueFamilyIndex() != physicalDevice->getPresentQueueFamilyIndex()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

// Initialization: Define image views creation function | SwapChain.cpp | Used by constructor and swap chain recreation | Creates image views for each swap chain image | Image View Setup - To enable Vulkan to access swap chain images | Depends on Vulkan device and swap chain images | Minimal computing power | Once per image view creation at [line 126 - SwapChain.cpp - createImageViews] | GPU
void SwapChain::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

// Initialization: Define depth resources creation function | SwapChain.cpp | Used by constructor and swap chain recreation | Creates depth image, allocates memory, and creates image view for depth buffering | Depth Resources Setup - To handle depth testing in rendering | Depends on Vulkan device, swap chain extent, and physical device | Moderate computing power | Once per depth resource creation at [line 135 - SwapChain.cpp - createDepthResources] | GPU
void SwapChain::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory
    );

    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// Initialization: Define function to find suitable depth format | SwapChain.cpp | Used by createDepthResources | Selects a supported depth format from candidates | Depth Format Selection - To ensure compatibility with depth attachments | Depends on PhysicalDevice and Vulkan device properties | Minimal computing power | Once per depth format search at [line 156 - SwapChain.cpp - findDepthFormat] | GPU
VkFormat SwapChain::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

// Initialization: Define supported format finder function | SwapChain.cpp | Used by findDepthFormat | Iterates through candidate formats to find a supported one | Format Support Check - To verify if a format meets required features | Depends on PhysicalDevice and Vulkan device properties | Minimal computing power | Once per supported format search at [line 166 - SwapChain.cpp - findSupportedFormat] | GPU
VkFormat SwapChain::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice->getPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
            (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

// Initialization: Define image creation function | SwapChain.cpp | Used by createDepthResources | Creates a Vulkan image, allocates memory, and binds it | Image Creation - To allocate and prepare images for rendering | Depends on PhysicalDevice and Vulkan device | Minimal computing power | Once per image creation at [line 185 - SwapChain.cpp - createImage] | GPU
void SwapChain::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        physicalDevice->findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

// Initialization: Define image view creation function | SwapChain.cpp | Used by createImageViews and createDepthResources | Creates a Vulkan image view for a given image | Image View Creation - To allow shaders to access image data | Depends on Vulkan device and image properties | Minimal computing power | Once per image view creation at [line 208 - SwapChain.cpp - createImageView] | GPU
VkImageView SwapChain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }

    return imageView;
}

// Initialization: Define swap surface format chooser function | SwapChain.cpp | Used by createSwapChain | Selects an appropriate surface format from available options | Surface Format Selection - To determine color format for swap chain images | Depends on PhysicalDevice and Vulkan device capabilities | Minimal computing power | Once per swap surface format selection at [line 231 - SwapChain.cpp - chooseSwapSurfaceFormat] | GPU
VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Prefer formats with VK_FORMAT_B8G8R8A8_SRGB and VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // Otherwise, return the first available format
    return availableFormats[0];
}

// Initialization: Define present mode chooser function | SwapChain.cpp | Used by createSwapChain | Selects an appropriate present mode from available options | Present Mode Selection - To determine how images are presented to the screen | Depends on PhysicalDevice and Vulkan device capabilities | Minimal computing power | Once per present mode selection at [line 242 - SwapChain.cpp - chooseSwapPresentMode] | GPU
VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Prefer VK_PRESENT_MODE_MAILBOX_KHR for low latency
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Initialization: Define swap extent chooser function | SwapChain.cpp | Used by createSwapChain | Determines the resolution of swap chain images based on window size and surface capabilities | Swap Extent Selection - To match swap chain image size with window size | Depends on GLFW window and PhysicalDevice capabilities | Minimal computing power | Once per swap extent selection at [line 259 - SwapChain.cpp - chooseSwapExtent] | GPU
VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        // The surface size is already defined
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

// Initialization: Define framebuffers creation function | SwapChain.cpp | Used by constructor and swap chain recreation | Creates framebuffers for each swap chain image view and depth image view | Framebuffer Creation - To hold rendered images for each swap chain image | Depends on Vulkan device, render pass, and image views | Moderate computing power | Once per framebuffer creation at [line 282 - SwapChain.cpp - createFramebuffers] | GPU
void SwapChain::createFramebuffers(VkRenderPass renderPass) {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}
