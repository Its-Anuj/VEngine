#pragma once

#include "Events.h"

namespace VEngine
{
    class Layer
    {
    public:
        Layer(const std::string &name) : _Name(name)
        {
            VENGINE_CORE_PRINTLN("[LAYERS]: " << _Name << " Layer!")
        }
        virtual ~Layer()
        {
            VENGINE_CORE_PRINTLN("[LAYERS]: " << _Name << " Destroyed!")
        }

        virtual void OnInit() = 0;
        virtual void OnUpdate() = 0;
        virtual void OnTerminate() = 0;
        virtual void OnEvent(Event &e) = 0;

    private:
        std::string _Name;
    };
} // namespace VEngine
