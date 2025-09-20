#pragma once

#include "VEngine.h"

namespace VEngine
{
    struct ApplicationSpec
    {
        std::string Name;
        Vec2 Dimensions;
        bool VSync = false;
    };

    class Application
    {
    public:
        Application() {}
        ~Application() {}

        void OnInit(const ApplicationSpec& InitSpec);

        void OnUpdate();

        void OnTerminate();

        void OnEvent(Event &e);

        void PushLayer(std::shared_ptr<Layer> layer) { _Stack.PushLayer(layer); }

    private:
        Window _Window;
        LayerStack _Stack;
        double _LastTime = 0.0f;
    };
} // namespace VEngine
