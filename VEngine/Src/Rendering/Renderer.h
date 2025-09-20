#pragma once

#include "Buffers.h"
#include "Shaders.h"
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
        static void Submit(std::shared_ptr<Shader>& shader, std::shared_ptr<VertexBuffer>& Vb, std::shared_ptr<IndexBuffer>& Ib) ;

        static void Begin(const RenderPassSpec& Spec);
        static void End();
        static void Submit();
        static void Present();
        static void Finish();

    private:
        RendererAPI *Api;

        Renderer() {}
        ~Renderer() {}
    };
} // namespace VEngine
