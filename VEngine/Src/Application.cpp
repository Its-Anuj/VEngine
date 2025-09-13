#include "VePCH.h"
#include "Application.h"

namespace VEngine
{
#define VENGINE_EVENT_CALLBACK_FN(x) {std::bind(&Application::x, this, std::placeholders::_1)}

    void Application::OnInit()
    {
        VEngine::WindowData Data;
        Data.Name = "VEditor";
        Data.Dimensions = VEngine::Vec2(800, 600);
        Data.VSync = true;

        _Window.Init(Data);
        _Window.SetEventCallback(VENGINE_EVENT_CALLBACK_FN(OnEvent));
    }

    void Application::OnUpdate()
    {
        while (!_Window.ShouldClose())
        {
            for (auto layer : _Stack)
                layer->OnUpdate();

            _Window.SwapBuffers();
        }
    }

    void Application::OnTerminate()
    {
        _Stack.Flush();
        _Window.Terminate();
    }

    void Application::OnEvent(Event &e)
    {
        if(e.GetType() == WindowCloseEvent::GetStaticType())
        {
            VENGINE_CORE_PRINTLN("Window Close")
        }

        for(auto layer : _Stack)
            layer->OnEvent(e);
    }

} // namespace VEngine
