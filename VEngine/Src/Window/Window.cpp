#include "VePCH.h"
#include "Window.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

#include "Input.h"

void VEngine_WindowCloseCallBack(GLFWwindow *window)
{
    VEngine::WindowFreePointer *Data = (VEngine::WindowFreePointer *)glfwGetWindowUserPointer(window);

    VEngine::WindowCloseEvent e;
    Data->Callback(e);
}

void VEngine_WindowSizeCallBack(GLFWwindow *window, int width, int height)
{
    VEngine::WindowFreePointer *Data = (VEngine::WindowFreePointer *)glfwGetWindowUserPointer(window);
    Data->Data.Dimensions = VEngine::Vec2(width, height);

    VEngine::WindowResizeEvent e(width, height);
    Data->Callback(e);
}

void VEngine_KeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    VEngine::WindowFreePointer *Data = (VEngine::WindowFreePointer *)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
            VEngine::Input::SetModState(VEngine::Input_mod_Shift, true);
        else if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
            VEngine::Input::SetModState(VEngine::Input_mod_Control, true);
        else if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
            VEngine::Input::SetModState(VEngine::Input_mod_Alt, true);
        else if (key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER)
            VEngine::Input::SetModState(VEngine::Input_mod_Super, true);
        else if (key == GLFW_KEY_CAPS_LOCK)
            VEngine::Input::SetModState(VEngine::Input_mod_CapsLock, true);
        else if (key == GLFW_KEY_NUM_LOCK)
            VEngine::Input::SetModState(VEngine::Input_mod_NumLock, true);

        VEngine::KeyPressedEvent e(key);
        Data->Callback(e);
    }
    else if (action == GLFW_REPEAT)
    {
        VEngine::KeyRepeatEvent e(key);
        Data->Callback(e);
    }
    else if (action == GLFW_RELEASE)
    {
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
            VEngine::Input::SetModState(VEngine::Input_mod_Shift, false);
        else if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
            VEngine::Input::SetModState(VEngine::Input_mod_Control, false);
        else if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
            VEngine::Input::SetModState(VEngine::Input_mod_Alt, false);
        else if (key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER)
            VEngine::Input::SetModState(VEngine::Input_mod_Super, false);
        else if (key == GLFW_KEY_CAPS_LOCK)
            VEngine::Input::SetModState(VEngine::Input_mod_CapsLock, false);
        else if (key == GLFW_KEY_NUM_LOCK)
            VEngine::Input::SetModState(VEngine::Input_mod_NumLock, false);

        VEngine::KeyReleasedEvent e(key);
        Data->Callback(e);
    }
}

void Vengine_MouseButtonCallBack(GLFWwindow *window, int button, int action, int mods)
{
    VEngine::WindowFreePointer *Data = (VEngine::WindowFreePointer *)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS)
    {
        VEngine::MouseButtonPressedEvent e(button);
        Data->Callback(e);
    }
    else if (action == GLFW_REPEAT)
    {
        VEngine::MouseButtonRepeatEvent e(button);
        Data->Callback(e);
    }
    else if (action == GLFW_RELEASE)
    {
        VEngine::MouseButtonReleasedEvent e(button);
        Data->Callback(e);
    }
}

void VEngine_CursorPosCallBack(GLFWwindow *window, double xpos, double ypos)
{
    VEngine::WindowFreePointer *Data = (VEngine::WindowFreePointer *)glfwGetWindowUserPointer(window);
    VEngine::Input::SetCursorPos({xpos, ypos});

    VEngine::CursorPosEvent e(xpos, ypos);
    Data->Callback(e);
}

void VEngine_MouseScrollCallBack(GLFWwindow *window, double xoffset, double yoffset)
{
    VEngine::WindowFreePointer *Data = (VEngine::WindowFreePointer *)glfwGetWindowUserPointer(window);

    VEngine::MouseScrollEvent e(xoffset, yoffset);
    Data->Callback(e);
}

namespace VEngine
{
    static bool GLFW_INIT = false;

    Window::Window(const WindowData &Data)
    {
        _Data.Data.Name = Data.Name;
        _Data.Data.Dimensions = Data.Dimensions;
        _Data.Data.VSync = Data.VSync;

        if (GLFW_INIT == false)
        {
            int Success = glfwInit();
            if (Success == GLFW_FALSE)
                VENGINE_ERROR("GLFW init couldnot be done!")
        }
        GLFW_INIT = true;
    }

    Window::~Window()
    {
        if (_Window != nullptr)
            Terminate();

        if (GLFW_INIT)
        {
            glfwTerminate();
            GLFW_INIT = false;
        }
    }

    void Window::Terminate()
    {
        glfwDestroyWindow(_Window);
        _Window = nullptr;
        VENGINE_CORE_PRINTLN("[WINDOW] Terminated: " << _Data.Data.Name)
    }

    void Window::SwapBuffers()
    {
        glfwSwapBuffers(_Window);
        glfwPollEvents();
    }

    bool Window::ShouldClose()
    {
        return glfwWindowShouldClose(_Window);
    }

    void Window::Init(const WindowData &Data)
    {
        _Data.Data = Data;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        _Window = glfwCreateWindow(800, 600, "VEditor", NULL, NULL);
        glfwMakeContextCurrent(_Window);
        VENGINE_CORE_PRINTLN("[WINDOW] Created: " << _Data.Data.Name << " with, " << _Data.Data.Dimensions.x << "x" << _Data.Data.Dimensions.y)

        glfwSwapInterval(Data.VSync);
        glfwSetWindowUserPointer(_Window, &_Data);

        // Callbacks
        glfwSetWindowCloseCallback(_Window, VEngine_WindowCloseCallBack);
        glfwSetWindowSizeCallback(_Window, VEngine_WindowSizeCallBack);
        glfwSetCursorPosCallback(_Window, VEngine_CursorPosCallBack);
        glfwSetMouseButtonCallback(_Window, Vengine_MouseButtonCallBack);
        glfwSetKeyCallback(_Window, VEngine_KeyCallBack);
        glfwSetScrollCallback(_Window, VEngine_MouseScrollCallBack);
    }

    void Window::SetVSync(bool state)
    {
        if (_Data.Data.VSync == state)
            return;
        _Data.Data.VSync = state;

        if (state)
            glfwSwapInterval(true);
        else
            glfwSwapInterval(false);
    }

    void *Window::GetWin32Surface()
    {
        return (void*)glfwGetWin32Window(_Window);
    }
} // namespace VEngine
