#pragma once

#include "RendererAPI.h"

namespace VEngine
{
    class Renderer
    {
    public:
        static Renderer &Get()
        {
            static Renderer Instance;
            return Instance;
        }

        static void Init(RenderAPIType Type, Window& window);
        static void Terminate();
        static void Render();
        static void FrameBufferResize(int x, int y);

    private:
        RendererAPI *Api;

        Renderer() {}
        ~Renderer() {}
    };
} // namespace VEngine
