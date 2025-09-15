#pragma once

#include "VEngine.h"
#include "VulkanRenderer.h"

namespace VEngine
{
    class Application
    {
    public:
        Application() {}
        ~Application() {}

        void OnInit();

        void OnUpdate();

        void OnTerminate();

        void OnEvent(Event &e);

        void PushLayer(std::shared_ptr<Layer> layer) { _Stack.PushLayer(layer); }

    private:
        Window _Window;
        VulkanRenderer Renderer;
        LayerStack _Stack;
    };
} // namespace VEngine
