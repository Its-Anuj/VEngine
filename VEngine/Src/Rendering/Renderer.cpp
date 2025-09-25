#include "VePCH.h"
#include "Window.h"
#include "Renderer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "ModernVulkan/VulkanRenderApi.h"

namespace VEngine
{
    static RenderAPIType _API;

    void Renderer::Init(const RendererInitSpec &RenderSpec)
    {
        if (RenderSpec.Type == RenderAPIType::VULKAN)
        {
            Get().Api = new VulkanRenderApi();

            VulkanRenderSpec Spec;
            Spec.EnableValidationLayer = true;
            Spec.Name = "VEngine";
            Spec.Win32Surface = RenderSpec.window->GetWin32Surface();
            Spec.FrameBufferSize.x = RenderSpec.window->GetFrameBufferSize().x;
            Spec.FrameBufferSize.y = RenderSpec.window->GetFrameBufferSize().y;
            Spec.InFrameFlightCount = 2;

            Get().Api->Init((void *)&Spec);
        }
        _API = RenderSpec.Type;
    }

    void Renderer::Terminate()
    {
        Get().Api->Terminate();
        delete Get().Api;
    }

    void Renderer::Render()
    {
        Get().Api->Render();
    }

    void Renderer::FrameBufferResize(int x, int y)
    {
        Get().Api->FrameBufferResize(x, y);
    }

    void Renderer::Begin(const RenderPassSpec &Spec)
    {
        Get().Api->Begin(Spec);
    }

    void Renderer::End()
    {
        Get().Api->End();
    }

    void Renderer::Submit(const Ref<VertexBuffer> &VB, const Ref<IndexBuffer>& IB)
    {
        Get().Api->Submit(VB, IB);
    }

    void Renderer::Present()
    {
        Get().Api->Present();
    }

    void Renderer::Finish()
    {
        Get().Api->Finish();
    }

    Ref<ResourceFactory> Renderer::__GetResouceFactory()
    {
        return Get().Api->GetResourceFactory();
    }
} // namespace VEngine
