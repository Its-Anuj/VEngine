#include "VePCH.h"
#include "Input.h"
#include "glfw3.h"
#include "InputCodes.h"

namespace VEngine
{
    void Input::Impl_Init(GLFWwindow *Window)
    {
        if (Get().IsActive() == true)
            VENGINE_ERROR("Input already initialized!")
        for (int i = 0; i < Input_key_Count; i++)
            KeyStates[i] = InputResult::INPUT_RELEASE;
        for (int i = 0; i < Input_mouse_Count; i++)
            MouseButtonStates[i] = InputResult::INPUT_RELEASE;

        _Window = Window;
    }

    InputResult Input::Impl_IsKeyPressed(Input_key KeyCode)
    {
        auto glfwinputid = InputKeysToGLFW(KeyCode);

        if (glfwinputid == -1)
            VENGINE_ERROR("Cant Conver to GLFW Mouse Button Code!")

        int Result = glfwGetKey(GetWindowHandle(), glfwinputid);

        InputResult LastState = KeyStates[KeyCode];

        if ((LastState == InputResult::INPUT_PRESS && Result == GLFW_PRESS) ||
            (LastState == InputResult::INPUT_PRESS && Result == GLFW_REPEAT))
            KeyStates[KeyCode] = InputResult::INPUT_REPEAT;
        else if (Result == GLFW_RELEASE)
            KeyStates[KeyCode] = InputResult::INPUT_RELEASE;
        else if (LastState == InputResult::INPUT_RELEASE && Result == GLFW_PRESS)
            KeyStates[KeyCode] = InputResult::INPUT_PRESS;

        return KeyStates[KeyCode];
    }

    InputResult Input::Impl_IsMouseButtonPressed(Input_mouse ButtonCode)
    {
        auto glfwinputid = InputMouseToGLFW(ButtonCode);

        if (glfwinputid == -1)
            VENGINE_ERROR("Cant Conver to GLFW Mouse Button Code!")

        int Result = glfwGetMouseButton(GetWindowHandle(), glfwinputid);

        InputResult LastState = MouseButtonStates[ButtonCode];

        if (LastState == InputResult::INPUT_RELEASE && Result == GLFW_PRESS)
            MouseButtonStates[ButtonCode] = InputResult::INPUT_PRESS;
        else if ((LastState == InputResult::INPUT_PRESS && Result == GLFW_RELEASE))
            MouseButtonStates[ButtonCode] = InputResult::INPUT_RELEASE;

        return MouseButtonStates[ButtonCode];
    }

    void Input::Impl_ShutDown()
    {
        _Window = nullptr;
    }

    bool Input::Impl_GetModState(Input_mod mod)
    {
        return ModStates[mod];
    }

    void Input::Impl_SetCursorPos(const Vec2 &Pos)
    {
        CursorPos = Pos;
    }

    void Input::Impl_SetModState(Input_mod mod, bool state)
    {
        ModStates[mod] = state;
    }

    int InputKeysToGLFW(Input_key key)
    {
        switch (key)
        {
        case Input_key_0:
            return GLFW_KEY_0;
        case Input_key_1:
            return GLFW_KEY_1;
        case Input_key_2:
            return GLFW_KEY_2;
        case Input_key_3:
            return GLFW_KEY_3;
        case Input_key_4:
            return GLFW_KEY_4;
        case Input_key_5:
            return GLFW_KEY_5;
        case Input_key_6:
            return GLFW_KEY_6;
        case Input_key_7:
            return GLFW_KEY_7;
        case Input_key_8:
            return GLFW_KEY_8;
        case Input_key_9:
            return GLFW_KEY_9;
        case Input_key_A:
            return GLFW_KEY_A;
        case Input_key_B:
            return GLFW_KEY_B;
        case Input_key_C:
            return GLFW_KEY_C;
        case Input_key_D:
            return GLFW_KEY_D;
        case Input_key_E:
            return GLFW_KEY_E;
        case Input_key_F:
            return GLFW_KEY_F;
        case Input_key_G:
            return GLFW_KEY_G;
        case Input_key_H:
            return GLFW_KEY_H;
        case Input_key_I:
            return GLFW_KEY_I;
        case Input_key_J:
            return GLFW_KEY_J;
        case Input_key_K:
            return GLFW_KEY_K;
        case Input_key_L:
            return GLFW_KEY_L;
        case Input_key_M:
            return GLFW_KEY_M;
        case Input_key_N:
            return GLFW_KEY_N;
        case Input_key_O:
            return GLFW_KEY_O;
        case Input_key_P:
            return GLFW_KEY_P;
        case Input_key_Q:
            return GLFW_KEY_Q;
        case Input_key_R:
            return GLFW_KEY_R;
        case Input_key_S:
            return GLFW_KEY_S;
        case Input_key_T:
            return GLFW_KEY_T;
        case Input_key_U:
            return GLFW_KEY_U;
        case Input_key_V:
            return GLFW_KEY_V;
        case Input_key_W:
            return GLFW_KEY_W;
        case Input_key_X:
            return GLFW_KEY_X;
        case Input_key_Y:
            return GLFW_KEY_Y;
        case Input_key_Z:
            return GLFW_KEY_Z;
        case Input_key_Space:
            return GLFW_KEY_SPACE;
        case Input_key_Escape:
            return GLFW_KEY_ESCAPE;
        case Input_key_Enter:
            return GLFW_KEY_ENTER;
        case Input_key_Tab:
            return GLFW_KEY_TAB;
        case Input_key_Backspace:
            return GLFW_KEY_BACKSPACE;
        case Input_key_Left:
            return GLFW_KEY_LEFT;
        case Input_key_Right:
            return GLFW_KEY_RIGHT;
        case Input_key_Up:
            return GLFW_KEY_UP;
        case Input_key_Down:
            return GLFW_KEY_DOWN;
        case Input_key_LeftShift:
            return GLFW_KEY_LEFT_SHIFT;
        case Input_key_RightShift:
            return GLFW_KEY_RIGHT_SHIFT;
        case Input_key_LeftControl:
            return GLFW_KEY_LEFT_CONTROL;
        case Input_key_RightControl:
            return GLFW_KEY_RIGHT_CONTROL;
        case Input_key_LeftAlt:
            return GLFW_KEY_LEFT_ALT;
        case Input_key_RightAlt:
            return GLFW_KEY_RIGHT_ALT;
        case Input_key_F1:
            return GLFW_KEY_F1;
        case Input_key_F2:
            return GLFW_KEY_F2;
        case Input_key_F3:
            return GLFW_KEY_F3;
        case Input_key_F4:
            return GLFW_KEY_F4;
        case Input_key_F5:
            return GLFW_KEY_F5;
        case Input_key_F6:
            return GLFW_KEY_F6;
        case Input_key_F7:
            return GLFW_KEY_F7;
        case Input_key_F8:
            return GLFW_KEY_F8;
        case Input_key_F9:
            return GLFW_KEY_F9;
        case Input_key_F10:
            return GLFW_KEY_F10;
        case Input_key_F11:
            return GLFW_KEY_F11;
        case Input_key_F12:
            return GLFW_KEY_F12;
        case Input_key_Unknown:
            return GLFW_KEY_UNKNOWN;
        case Input_key_Apostrophe:
            return GLFW_KEY_APOSTROPHE;
        case Input_key_Comma:
            return GLFW_KEY_COMMA;
        case Input_key_Minus:
            return GLFW_KEY_MINUS;
        case Input_key_Period:
            return GLFW_KEY_PERIOD;
        case Input_key_Slash:
            return GLFW_KEY_SLASH;
        case Input_key_Semicolon:
            return GLFW_KEY_SEMICOLON;
        case Input_key_Equal:
            return GLFW_KEY_EQUAL;
        case Input_key_LeftBracket:
            return GLFW_KEY_LEFT_BRACKET;
        case Input_key_Backslash:
            return GLFW_KEY_BACKSLASH;
        case Input_key_RightBracket:
            return GLFW_KEY_RIGHT_BRACKET;
        case Input_key_GraveAccent:
            return GLFW_KEY_GRAVE_ACCENT;
        case Input_key_World1:
            return GLFW_KEY_WORLD_1;
        case Input_key_World2:
            return GLFW_KEY_WORLD_2;
        case Input_key_Insert:
            return GLFW_KEY_INSERT;
        case Input_key_Delete:
            return GLFW_KEY_DELETE;
        case Input_key_PageUp:
            return GLFW_KEY_PAGE_UP;
        case Input_key_PageDown:
            return GLFW_KEY_PAGE_DOWN;
        case Input_key_Home:
            return GLFW_KEY_HOME;
        case Input_key_End:
            return GLFW_KEY_END;
        case Input_key_CapsLock:
            return GLFW_KEY_CAPS_LOCK;
        case Input_key_ScrollLock:
            return GLFW_KEY_SCROLL_LOCK;
        case Input_key_NumLock:
            return GLFW_KEY_NUM_LOCK;
        case Input_key_PrintScreen:
            return GLFW_KEY_PRINT_SCREEN;
        case Input_key_Pause:
            return GLFW_KEY_PAUSE;
        case Input_key_F13:
            return GLFW_KEY_F13;
        case Input_key_F14:
            return GLFW_KEY_F14;
        case Input_key_F15:
            return GLFW_KEY_F15;
        case Input_key_F16:
            return GLFW_KEY_F16;
        case Input_key_F17:
            return GLFW_KEY_F17;
        case Input_key_F18:
            return GLFW_KEY_F18;
        case Input_key_F19:
            return GLFW_KEY_F19;
        case Input_key_F20:
            return GLFW_KEY_F20;
        case Input_key_F21:
            return GLFW_KEY_F21;
        case Input_key_F22:
            return GLFW_KEY_F22;
        case Input_key_F23:
            return GLFW_KEY_F23;
        case Input_key_F24:
            return GLFW_KEY_F24;
        case Input_key_F25:
            return GLFW_KEY_F25;
        case Input_key_Kp0:
            return GLFW_KEY_KP_0;
        case Input_key_Kp1:
            return GLFW_KEY_KP_1;
        case Input_key_Kp2:
            return GLFW_KEY_KP_2;
        case Input_key_Kp3:
            return GLFW_KEY_KP_3;
        case Input_key_Kp4:
            return GLFW_KEY_KP_4;
        case Input_key_Kp5:
            return GLFW_KEY_KP_5;
        case Input_key_Kp6:
            return GLFW_KEY_KP_6;
        case Input_key_Kp7:
            return GLFW_KEY_KP_7;
        case Input_key_Kp8:
            return GLFW_KEY_KP_8;
        case Input_key_Kp9:
            return GLFW_KEY_KP_9;
        case Input_key_KpDecimal:
            return GLFW_KEY_KP_DECIMAL;
        case Input_key_KpDivide:
            return GLFW_KEY_KP_DIVIDE;
        case Input_key_KpMultiply:
            return GLFW_KEY_KP_MULTIPLY;
        case Input_key_KpSubtract:
            return GLFW_KEY_KP_SUBTRACT;
        case Input_key_KpAdd:
            return GLFW_KEY_KP_ADD;
        case Input_key_KpEnter:
            return GLFW_KEY_KP_ENTER;
        case Input_key_KpEqual:
            return GLFW_KEY_KP_EQUAL;
        case Input_key_LeftSuper:
            return GLFW_KEY_LEFT_SUPER;
        case Input_key_RightSuper:
            return GLFW_KEY_RIGHT_SUPER;
        case Input_key_Menu:
            return GLFW_KEY_MENU;

        default:
            return -1;
        }
    }

    int InputMouseToGLFW(Input_mouse mouse)
    {
        switch (mouse)
        {
        case Input_mouse_Left:
            return GLFW_MOUSE_BUTTON_LEFT;
        case Input_mouse_Right:
            return GLFW_MOUSE_BUTTON_RIGHT;
        case Input_mouse_Middle:
            return GLFW_MOUSE_BUTTON_MIDDLE;
        case Input_mouse_Button4:
            return GLFW_MOUSE_BUTTON_4;
        case Input_mouse_Button5:
            return GLFW_MOUSE_BUTTON_5;
        case Input_mouse_Button6:
            return GLFW_MOUSE_BUTTON_6;
        case Input_mouse_Button7:
            return GLFW_MOUSE_BUTTON_7;
        case Input_mouse_Button8:
            return GLFW_MOUSE_BUTTON_8;
        default:
            return -1;
        }
    }

    int InputModsToGLFW(Input_mod mod)
    {
        switch (mod)
        {
        case Input_mod_Shift:
            return GLFW_MOD_SHIFT;
        case Input_mod_Control:
            return GLFW_MOD_CONTROL;
        case Input_mod_Alt:
            return GLFW_MOD_ALT;
        case Input_mod_Super:
            return GLFW_MOD_SUPER;
        case Input_mod_CapsLock:
            return GLFW_MOD_CAPS_LOCK;
        case Input_mod_NumLock:
            return GLFW_MOD_NUM_LOCK;
        default:
            return 0;
        }
    }

} // namespace VEngine
