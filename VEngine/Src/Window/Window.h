#pragma once

#include "Events.h"

/*
    Contains Window and WindowStack classes
    Window Class:
        The basic class having authority to create and destory windows upon which we can render or show error messages
        and such
    WindowStack Class:
        Holds all the Window in a stack and handles their creaton and deletion
*/

struct GLFWwindow;

namespace VEngine
{
    struct Vec2
    {
        float x, y;

        Vec2(float px = 0.0f, float py = 0.0f) : x(px), y(py) {}

        Vec2 &operator=(const Vec2 &other)
        {
            this->x = other.x;
            this->y = other.y;
            return *this;
        }
    };

    struct WindowData
    {
        std::string Name;
        Vec2 Dimensions;
        bool VSync = false;

        WindowData(const std::string &name = "Untitled", const Vec2 &dim = Vec2()) : Name(name), Dimensions(dim) {}
    };

    struct WindowFreePointer
    {
        WindowData Data;
        std::function<void(Event &)> Callback;
    };
    
    class Window
    {
    public:
        Window(const WindowData &Data = WindowData());
        ~Window();

        void Terminate();

        void SwapBuffers();
        bool ShouldClose();

        void Init(const WindowData &Data);
        void SetVSync(bool state);
        void SetEventCallback(const std::function<void(Event &)> &Callback) { _Data.Callback = Callback; }

    private:
        GLFWwindow *_Window = nullptr;

        WindowFreePointer _Data;
    };
} // namespace VEngine
