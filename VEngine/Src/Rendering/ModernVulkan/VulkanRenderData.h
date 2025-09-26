#pragma once

#define PRINTLN(x)      \
    {                   \
        std::cout << x; \
    }
#define PRINTLN(x)              \
    {                           \
        std::cout << x << "\n"; \
    }
#define VULKAN_SUCCESS_ASSERT(x, errmsg)      \
    {                                         \
        if (x != VK_SUCCESS)                  \
            throw std::runtime_error(errmsg); \
    }

namespace VEngine
{
    struct VulkanRenderData
    {
        // Inital Setups
        VkInstance Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkSurfaceKHR SurfaceKHR;

        // Devices
        std::vector<VulkanPhysicalDevice> PhysicalDevices;
        int ActivePhysicalDeviceIndex = 0;

        VulkanDevice Device;

        VmaAllocator Allocator;

        VkSwapchainKHR SwapChain;
        VkFormat Format;
        VkExtent2D Extent;

        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;

        struct
        {
            int x, y;
        } FrameBufferSize;

        VkCommandPool GraphicsCommandPool, TransferCommandPool;
        std::vector<VkCommandBuffer> GraphicsCommandBuffers;
        std::vector<VkSemaphore> ImageAvailableSemaphores;
        std::vector<VkSemaphore> RenderFinishedSemaphores;
        std::vector<VkFence> InFlightFences;
        uint32_t CurrentFrame = 0;
        uint32_t CurrentImageIndex = 0;
        bool FrameBufferChanged = false;

        VulkanGraphicsPipeline GraphicsPipeline;

        VkDescriptorPool DescriptorPool;
        VkDescriptorSetLayout DescriptorSetLayout;
        std::vector<Scope<VulkanUniformBuffer>> UniformBuffers;
        std::vector<VkDescriptorSet> DescriptorSets;

        VulkanTextures Texture;
        VulkanTextureDescriptor TextureDescriptor;
    };

    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
} // namespace VEngine
