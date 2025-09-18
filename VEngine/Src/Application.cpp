#include "VePCH.h"
#include "Application.h"

namespace VEngine
{
#define VENGINE_EVENT_CALLBACK_FN(x) {std::bind(&Application::x, this, std::placeholders::_1)}

    void Application::OnInit()
    {
        VENGINE_DEBUG_TIMER("Initialization!")

        VEngine::WindowData Data;
        Data.Name = "VEditor";
        Data.Dimensions = VEngine::Vec2(800, 600);
        Data.VSync = true;

        _Window.Init(Data);
        _Window.SetEventCallback(VENGINE_EVENT_CALLBACK_FN(OnEvent));

        Input::Init(_Window.GetRawHandle());
        Renderer::Init(RenderAPIType::VULKAN, _Window);

        {
            // VENGINE_DEBUG_TIMER("Vulkan Init")

            // VulkanRenderSpec Spec;
            // Spec.Name = "VEngine";
            // Spec.Version = VulkanSupportedVersions::V_1_0;
            // Spec.EnableValidationLayer = true;

            // Spec.RequirerdExtensions.push_back("VK_KHR_surface");
            // Spec.RequirerdExtensions.push_back("VK_KHR_win32_surface");
            // Spec.Win32Surface = _Window.GetWin32Surface();
            // auto fb = _Window.GetFrameBufferSize();
            // Spec.FrameBufferWidth = _Window.GetFrameBufferSize().x;
            // Spec.FrameBufferHeight = _Window.GetFrameBufferSize().y;

            // Renderer.Init(Spec);
        }
    }

    void Application::OnUpdate()
    {
        while (!_Window.ShouldClose())
        {
            for (auto layer : _Stack)
                layer->OnUpdate();

            _Window.SwapBuffers();
            Renderer::Render();
        }
    }

    void Application::OnTerminate()
    {
        VENGINE_DEBUG_TIMER("Terminate!")

        _Stack.Flush();
        // Renderer::Terminate();
        Input::ShutDown();
        _Window.Terminate();
    }

    void Application::OnEvent(Event &e)
    {
        if (e.GetType() == WindowCloseEvent::GetStaticType())
            VENGINE_CORE_PRINTLN("Window Close")

        for (auto layer : _Stack)
            layer->OnEvent(e);
    }

} // namespace VEngine
