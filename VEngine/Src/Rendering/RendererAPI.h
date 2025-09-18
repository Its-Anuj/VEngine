#pragma once

namespace VEngine
{
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

    private:
    };
} // namespace VEngine
