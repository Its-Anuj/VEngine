#include "VePCH.h"
#include "Application.h"
#include "glfw3.h"

namespace VEngine
{
    class Editor : public Layer
    {
    public:
        Editor()
            : Layer("Editor")
        {
        }
        ~Editor()
        {
        }

        void OnInit() override {}
        
        void OnUpdate() override
        {
            if(Input::IsKeyPressed(Input_key_W) == InputResult::INPUT_PRESS && Input::GetModState(Input_mod_Shift))
            {
                VENGINE_CORE_PRINTLN("W Press")
            }
            if(Input::IsKeyPressed(Input_key_W) == InputResult::INPUT_REPEAT)
            {
                VENGINE_CORE_PRINTLN("W Repeat")
            }
        }

        void OnTerminate() override {}
        void OnEvent(Event &e) override
        {
            if (e.GetType() == WindowResizeEvent::GetStaticType())
            {
                VENGINE_APP_PRINTLN("Window Resize")
            }
        }

    private:
    };
} // namespace VEngine

int main(int argc, char const *argv[])
{
    VEngine::Application app;
    app.OnInit();
    app.PushLayer(std::make_shared<VEngine::Editor>());

    app.OnUpdate();
    app.OnTerminate();

    return 0;
}
