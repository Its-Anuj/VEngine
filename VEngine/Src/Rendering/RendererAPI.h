#pragma once

#include "Core.h"
#include "ResourceFactory.h"

namespace VEngine
{
    struct RenderPassSpec
    {
        struct
        {
            float x, y, z, w;
        } ClearColor;
    };

    enum class RenderAPIType
    {
        VULKAN,
        OPENGL
    };
    
    class RendererAPI
    {
    public:
        RendererAPI() {}
        ~RendererAPI() {}

        // Respective to own renderer api spec
        virtual void Init(void *Spec) = 0;
        virtual void Terminate() = 0;
        virtual void Render() = 0;
        virtual void FrameBufferResize(int x, int y) = 0;

        virtual void Submit(const Ref<VertexBuffer> &VB, const Ref<IndexBuffer>& IB) = 0;
        virtual void Present() = 0;
        virtual void Begin(const RenderPassSpec& Spec) = 0;
        virtual void End() = 0;
        virtual void Finish() = 0;

        virtual Ref<ResourceFactory> GetResourceFactory() = 0;
    private:
    };
} // namespace VEngine
