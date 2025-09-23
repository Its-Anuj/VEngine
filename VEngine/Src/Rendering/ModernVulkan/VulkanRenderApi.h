#pragma once

#include "RendererAPI.h"
#include "Buffers.h"
struct VkDebugUtilsMessengerCreateInfoEXT;

namespace VEngine
{
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

    struct VulkanResourceFactory : public ResourceFactory
    {
    public:
        VulkanResourceFactory(VulkanRenderData *data) : _Data(data) {}
        ~VulkanResourceFactory();

        void Terminate();

        Ref<VertexBuffer> CreateVertexBuffer(const VertexBufferDesc &desc) override;
        Ref<IndexBuffer> CreateIndexBuffer(const IndexBufferDesc &desc) override;
        Ref<Shader> CreateGraphicsPipeline(const GraphicsShaderDesc &desc) override;

    private:
        VulkanRenderData *_Data;
    };

    class VulkanRenderApi : public RendererAPI
    {
    public:
        // Respective to own renderer api spec
        void Init(void *Spec) override;
        void Terminate() override;
        void Render() override ;
        void FrameBufferResize(int x, int y) override {}

        void Submit() override ;
        void Present() override ;
        void Begin(const RenderPassSpec &Spec) override ;
        void End() override;
        void Finish() override {}

        Ref<ResourceFactory> GetResourceFactory() override { return _ResourceFactory; }

    private:
        void _CreateInstance();
        void _CreateDebugMessenger();
        void _PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void _CreateWin32Surface();

        void _CreatePhysicalDevice();
        void _FindSuitablePhysicalDevice();

        void _CreateLogicalDevice();

        void _CreateSwapChain();

        void _CreateGraphiscPipeline();

        void _CreateCommandPool();
        void _CreateCommandBuffer();

        void _CreateSyncObjects();

        void _RecordCommandBuffer(uint32_t imageindex);

    private:
        VulkanRenderData *_Data;
        VulkanRenderSpec _Spec;
        Ref<VulkanResourceFactory> _ResourceFactory;
    };
} // namespace VEngine
