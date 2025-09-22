#pragma once

struct VkDebugUtilsMessengerCreateInfoEXT;

#include "RendererAPI.h"

namespace VEngine
{
    struct VulkanRendererData;
    struct QueueFamilyIndices;
    struct VulkanDevice;
    struct VulkanPhysicalDevice;
    struct VulkanSwapChain;
    struct VulkanBuffer;

    struct VulkanRuntimeData
    {
        VulkanDevice *ActiveDevice;
        VulkanPhysicalDevice *ActivePhysicalDevice;
        struct
        {
            float x, y;
        } FrameBufferSize;
        VkSurfaceKHR *Surface;
        VulkanSwapChain *SwapChain;
    };

    VulkanRuntimeData *GetRuntimeData();

    enum class VulkanSupportedVersions
    {
        V_1_0
    };

    struct VulkanRenderSpec
    {
        std::string Name;
        VulkanSupportedVersions Version;
        std::vector<const char *> RequirerdExtensions;
        std::vector<const char *> RequirerdLayers;
        std::vector<const char *> DeviceRequirerdExtensions;
        bool EnableValidationLayer = false;
        void *Win32Surface;
        int FrameBufferWidth, FrameBufferHeight;
        int FramesInFlightCount;
    };

    class VulkanRenderer : public RendererAPI
    {
    public:
        VulkanRenderer();
        ~VulkanRenderer();

        virtual void Init(void *Spec) override;
        virtual void Terminate() override;
        virtual void Render() override;
        virtual void FrameBufferResize(int x, int y) override;

        virtual void Begin(const RenderPassSpec &Spec) override;
        virtual void End() override;
        virtual void Submit(std::shared_ptr<VertexBuffer> &vb, std::shared_ptr<IndexBuffer> &ib) override;

        virtual void Present() override;

        virtual void Finish() override;
        void CopyBuffer(VulkanBuffer &SrcBuffer, VulkanBuffer &DstBuffer, uint32_t Size);

    private:
        void _CreateInstance();
        void _CreateSuitablePhysicalDevice();
        void _CreateSuitableLogicalDevice();
        void _CreateWindowSurface();
        void _CreateSwapChain();
        void _CreateDescriptorSetLayout();
        void _CreateDescriptorSets();
        void _CreateDescriptorPool();
        void _CreateGraphicsPipeline();
        void _CreateFrameBuffers();
        void _CreateUniformBuffers();
        void _CreateCommandPool();
        void _CreateCommandBuffer();
        void _CreateSyncObject();

        void _UpdateUniformBuffer(uint32_t currentImage);
        void _CleanUpSwapChain();
        void _ReCreateSwapChain();

        void _PopulatePhysicalDevices();
        void _CheckSuitablePhysicalDevice();
        void _CreateRenderPass();

        void _CreateDebugMessenger();
        void _PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        bool _CheckSupportLayer(const std::vector<const char *> &layers);
        bool _CheckSupportExts(const std::vector<const char *> &exts);

    private:
        VulkanRendererData *_Data;
        VulkanRenderSpec _Spec;
    };
} // namespace VEngine1
