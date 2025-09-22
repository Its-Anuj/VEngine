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

    void Renderer::Submit(std::shared_ptr<Shader> &shader, std::shared_ptr<VertexBuffer> &Vb, std::shared_ptr<IndexBuffer> &Ib)
    {
        Get().Api->Submit(Vb, Ib);
    }

    void Renderer::Begin(const RenderPassSpec &Spec)
    {
        Get().Api->Begin(Spec);
    }

    void Renderer::End()
    {
        Get().Api->End();
    }

    void Renderer::Submit()
    {
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

    std::shared_ptr<VertexBuffer> VEngine::VertexBuffer::Create(const VertexBufferDesc& desc)
    {
        if (_API == RenderAPIType::VULKAN)
        {
            return Renderer::__GetResouceFactory()->CreateVertexBuffer(desc);
        }
    }

    std::shared_ptr<IndexBuffer> VEngine::IndexBuffer::Create(const IndexBufferDesc& desc)
    {
        if (_API == RenderAPIType::VULKAN)
        {
            return Renderer::__GetResouceFactory()->CreateIndexBuffer(desc);
        }
    }

} // namespace VEngine
