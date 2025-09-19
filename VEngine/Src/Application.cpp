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

        {
            VENGINE_DEBUG_TIMER("Vulkan Init")
            Renderer::Init(RenderAPIType::VULKAN, _Window);
        }
    }

    void Application::OnUpdate()
    {
        while (!_Window.ShouldClose())
        {
            // do after changin in frame flight count
            double StartTime = GetWindowTime();
            TimeStep ts = (StartTime - _LastTime);
            _LastTime = StartTime;

            for (auto layer : _Stack)
                layer->OnUpdate(ts);

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
        if (e.GetType() == FrameBufferResizeEvent::GetStaticType())
        {
            auto &fbe = static_cast<FrameBufferResizeEvent &>(e);
            VENGINE_CORE_PRINTLN("FrameBuffer Resize")
            Renderer::FrameBufferResize(fbe.GetX(), fbe.GetY());
        }
        for (auto layer : _Stack)
            layer->OnEvent(e);
    }

} // namespace VEngine
