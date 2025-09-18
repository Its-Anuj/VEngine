#include "VePCH.h"
#include "Window.h"
#include "Renderer.h"

#include "VulkanRenderer.h"

namespace VEngine
{
    void Renderer::Init(RenderAPIType Type, Window &window)
    {
        if (Type == RenderAPIType::VULKAN)
        {
            Get().Api = new VulkanRenderer();

            VulkanRenderSpec Spec;
            Spec.Name = "VEngine";
            Spec.Version = VulkanSupportedVersions::V_1_0;
            Spec.EnableValidationLayer = true;

            Spec.RequirerdExtensions.push_back("VK_KHR_surface");
            Spec.RequirerdExtensions.push_back("VK_KHR_win32_surface");
            Spec.Win32Surface = window.GetWin32Surface();
            auto fb = window.GetFrameBufferSize();
            Spec.FrameBufferWidth = window.GetFrameBufferSize().x;
            Spec.FrameBufferHeight = window.GetFrameBufferSize().y;

            Get().Api->Init((void *)&Spec);
        }
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
        Get().Api->FrameBufferResize(x,y);
    }
} // namespace VEngine
