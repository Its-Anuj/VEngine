#pragma once

#include "RendererAPI.h"
struct VkDebugUtilsMessengerCreateInfoEXT;

namespace VEngine
{
    struct VulkanVersion
    {
        int16_t Max, Min, Patch;
    };

    struct VulkanRenderSpec
    {
        bool EnableValidationLayer = true;
        std::string Name = " ";
        std::vector<const char *> DeviceExtensions;
        std::vector<const char *> InstanceExtensions;
        std::vector<const char *> InstanceLayers;
        void *Win32Surface;

        struct
        {
            int x, y;
        } FrameBufferSize;
        int InFrameFlightCount = 0;

        VulkanRenderSpec() {}
    };

    struct VulkanRenderData;
    struct VulkanResourceFactory;

    class VulkanRenderApi : public RendererAPI
    {
    public:
        // Respective to own renderer api spec
        void Init(void *Spec) override;
        void Terminate() override;
        void Render() override;
        void FrameBufferResize(int x, int y) override;

        void Submit(const Ref<VertexBuffer> &VB, const Ref<IndexBuffer> &IB) override;
        void Present() override;
        void Begin(const RenderPassSpec &Spec) override;
        void End() override;
        void Finish() override;

        Ref<ResourceFactory> GetResourceFactory() override;

    private:
        void _CreateInstance();
        void _CreateDebugMessenger();
        void _PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void _CreateWin32Surface();

        void _CreatePhysicalDevice();
        void _FindSuitablePhysicalDevice();

        void _CreateLogicalDevice();

        void _CreateSwapChain();
        void _RecreateSwapChain();
        void _DestroySwapChain();

        void _CreateGraphiscPipeline();

        void _CreateCommandPool();
        void _CreateCommandBuffer();

        void _CreateSyncObjects();

        void _CreateDescriptorSetLayout();
        void _CreateUniformBuffers();
        void _UploadUniformBuffer();
        void _CreateDescriptorPool();
        void _CreateDescriptorSets();

        void _CreateTextures();
        void _TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage Image, VkImageLayout NewLayout, VkImageLayout OldLayout);
        VkImageView _CreateTextureImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        VkSampler _CreateTextureSampler();
        void _CreateTextureDescriptorResources();

    private:
        VulkanRenderData *_Data;
        VulkanRenderSpec _Spec;
        Ref<VulkanResourceFactory> _ResourceFactory;
    };
} // namespace VEngine
