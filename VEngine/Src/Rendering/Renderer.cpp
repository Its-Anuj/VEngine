#include "VePCH.h"
#include "Window.h"
#include "Renderer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VulkanRenderer.h"
#include "VulkanShaders.h"
#include "VulkanBuffers.h"

namespace VEngine
{
    static RenderAPIType _API;

    void Renderer::Init(const RendererInitSpec &RenderSpec)
    {
        if (RenderSpec.Type == RenderAPIType::VULKAN)
        {
            Get().Api = new VulkanRenderer();

            VulkanRenderSpec Spec;
            Spec.Name = "VEngine";
            Spec.Version = VulkanSupportedVersions::V_1_0;
            Spec.EnableValidationLayer = true;

            Spec.RequirerdExtensions.push_back("VK_KHR_surface");
            Spec.RequirerdExtensions.push_back("VK_KHR_win32_surface");
            Spec.Win32Surface = RenderSpec.window->GetWin32Surface();
            auto fb = RenderSpec.window->GetFrameBufferSize();
            Spec.FrameBufferWidth = RenderSpec.window->GetFrameBufferSize().x;
            Spec.FrameBufferHeight = RenderSpec.window->GetFrameBufferSize().y;
            Spec.FramesInFlightCount = RenderSpec.FramesInFlightCount;

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

    std::shared_ptr<VertexBuffer> VEngine::VertexBuffer::Create(float *vertices, uint32_t size, BufferTypes BType)
    {
        if (_API == RenderAPIType::VULKAN)
        {
            std::shared_ptr<VulkanVertexBuffer> VB = std::make_shared<VulkanVertexBuffer>();
            VB->Init(vertices, size, BType);
            return VB;
        }
    }

    std::shared_ptr<IndexBuffer> VEngine::IndexBuffer::Create(void *data, int Uintcount, IndexBufferType Type, BufferTypes BType)
    {
        if (_API == RenderAPIType::VULKAN)
        {
            std::shared_ptr<VulkanIndexBuffer> IB = std::make_shared<VulkanIndexBuffer>();
            IB->Init(data, Uintcount, Type, BType);
            return IB;
        }
    }

    std::shared_ptr<Shader> VEngine::Shader::Create(const ShaderSpec &Spec)
    {
        if (_API == RenderAPIType::VULKAN)
        {
            std::shared_ptr<VulkanShader> VB = std::make_shared<VulkanShader>();
            VB->Init(Spec);
            return VB;
        }
    }
} // namespace VEngine
