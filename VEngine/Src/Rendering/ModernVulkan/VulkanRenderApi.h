#pragma once

#include "RendererAPI.h"
#include "Buffers.h"

namespace VEngine
{
    struct VulkanRenderSpec
    {
        bool EnableValidationLayer = true;
        std::string Name = " ";

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
        void Render() override {}
        void FrameBufferResize(int x, int y) override {}

        void Submit(std::shared_ptr<VertexBuffer> &vb, std::shared_ptr<IndexBuffer> &ib) override {}
        void Present() override {}
        void Begin(const RenderPassSpec &Spec) override {}
        void End() override {}
        void Finish() override {}

        Ref<ResourceFactory> GetResourceFactory() override { return _ResourceFactory; }

    private:
        void _CreateInstance();

    private:
        VulkanRenderData *_Data;
        VulkanRenderSpec _Spec;
        Ref<VulkanResourceFactory> _ResourceFactory;
    };
} // namespace VEngine
