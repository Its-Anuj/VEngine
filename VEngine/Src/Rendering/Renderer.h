#pragma once

#include "RendererAPI.h"

namespace VEngine
{
    struct RendererInitSpec
    {
        RenderAPIType Type;
        Window* window;
        // Sepcifices the max number of in flight frames
        int FramesInFlightCount;
        
    };

    class Renderer
    {
    public:
        static Renderer &Get()
        {
            static Renderer Instance;
            return Instance;
        }

        static void Init(const RendererInitSpec& RenderSpec);
        static void Terminate();
        static void Render();
        static void FrameBufferResize(int x, int y);

        static void Begin(const RenderPassSpec& Spec);
        static void End();
        static void Submit(const Ref<VertexBuffer> &VB, const Ref<IndexBuffer>& IB);
        static void Present();
        static void Finish();
        static Ref<ResourceFactory> __GetResouceFactory();

    private:
        RendererAPI *Api;

        Renderer() {}
        ~Renderer() {}
    };
} // namespace VEngine
