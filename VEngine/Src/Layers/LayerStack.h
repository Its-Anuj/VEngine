#pragma once

#include "Layer.h"

namespace VEngine
{
    class LayerStack
    {
    public:
        LayerStack() { VENGINE_CORE_PRINTLN("[LAYER_STACK]: Init") }
        virtual ~LayerStack();

        void PushLayer(std::shared_ptr<Layer> layer);

        void Flush();

        using Iterator = std::vector<std::shared_ptr<Layer>>::iterator;
        using Const_Iterator = std::vector<std::shared_ptr<Layer>>::const_iterator;

        Iterator begin() { return _Layers.begin(); }
        Iterator end() { return _Layers.end(); }

        Const_Iterator begin() const { return _Layers.begin(); }
        Const_Iterator end() const { return _Layers.end(); }

    private:
        std::vector<std::shared_ptr<Layer>> _Layers;
    };
} // namespace VEngine
