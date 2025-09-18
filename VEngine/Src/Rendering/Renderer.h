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

    private:
        RendererAPI *Api;

        Renderer() {}
        ~Renderer() {}
    };
} // namespace VEngine
