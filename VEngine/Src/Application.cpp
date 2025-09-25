#include "VePCH.h"
#include "Application.h"

namespace VEngine
{
#define VENGINE_EVENT_CALLBACK_FN(x) {std::bind(&Application::x, this, std::placeholders::_1)}

    void Application::OnInit(const ApplicationSpec &InitSpec)
    {
        VENGINE_DEBUG_TIMER("Initialization!")

        VEngine::WindowData Data;
        Data.Name = InitSpec.Name;
        Data.Dimensions = InitSpec.Dimensions;
        Data.VSync = InitSpec.VSync;

        _Window.Init(Data);
        _Window.SetEventCallback(VENGINE_EVENT_CALLBACK_FN(OnEvent));

        Input::Init(_Window.GetRawHandle());

        {
            VENGINE_DEBUG_TIMER("Vulkan Init")
            RendererInitSpec RenderSpec;
            RenderSpec.Type = RenderAPIType::VULKAN;
            RenderSpec.window = &_Window;
            RenderSpec.FramesInFlightCount = 2;
            Renderer::Init(RenderSpec);
        }
    }

    void Application::OnUpdate()
    {
        while (!_Window.ShouldClose())
        {
            _Window.PollEvents();
            auto Startime = GetWindowTime();
            TimeStep ts = Startime - _LastTime;
            _LastTime = Startime;

            for (auto layer : _Stack)
                layer->OnUpdate(ts);

            _Window.SwapBuffers();
        }
    }

    void Application::OnTerminate()
    {
        VENGINE_DEBUG_TIMER("Terminate!")
        Renderer::Finish();

        _Stack.Flush();
        Renderer::Terminate();
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
