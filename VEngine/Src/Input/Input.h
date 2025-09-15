#pragma once

#include "InputCodes.h"

struct GLFWwindow;

#include "Maths.h"

namespace VEngine
{
    enum class InputResult
    {
        INPUT_PRESS,
        INPUT_REPEAT,
        INPUT_RELEASE
    };

    class Input
    {
    public:
        static Input &Get()
        {
            static Input instance;
            return instance;
        }

        static void Init(GLFWwindow *Window) { Get().Impl_Init(Window); }
        static void ShutDown() { Get().Impl_ShutDown(); }

        static InputResult IsKeyPressed(Input_key KeyCode) { return Get().Impl_IsKeyPressed(KeyCode); }
        static InputResult IsMouseButtonPressed(Input_mouse KeyCode) { return Get().Impl_IsMouseButtonPressed(KeyCode); }
        static bool GetModState(Input_mod mod) { return Get().Impl_GetModState(mod); }

        static GLFWwindow *GetWindow() { return Get().GetWindow(); }

        static bool IsActive() { return GetWindowHandle() != nullptr; }

        static GLFWwindow *GetWindowHandle() { return Get()._Window; }
        static void SetModState(Input_mod mod, bool state) { Get().Impl_SetModState(mod, state); }

        static const Vec2 &GetCursorPos() { return Get().Impl_GetCursorPos(); }
        static void SetCursorPos(const Vec2 &Pos) { Get().Impl_SetCursorPos(Pos); }

    private:
        void Impl_Init(GLFWwindow *Window);
        InputResult Impl_IsKeyPressed(Input_key KeyCode);
        InputResult Impl_IsMouseButtonPressed(Input_mouse ButtonCode);
        void Impl_ShutDown();

        bool Impl_GetModState(Input_mod mod);
        void Impl_SetModState(Input_mod mod, bool state);

        const Vec2 &Impl_GetCursorPos() { return CursorPos; }
        void Impl_SetCursorPos(const Vec2 &Pos);

    private:
        GLFWwindow *_Window = nullptr;
        InputResult KeyStates[Input_key_Count];
        InputResult MouseButtonStates[Input_mouse_Count];
        bool ModStates[Input_mod_Count];
        Vec2 CursorPos;

        Input() {}
        ~Input() {}
    };
} // namespace VEngine
