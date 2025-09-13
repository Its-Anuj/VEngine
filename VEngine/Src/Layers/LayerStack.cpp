#include "VePCH.h"
#include "LayerStack.h"

namespace VEngine
{
    LayerStack::~LayerStack()
    {
        VENGINE_CORE_PRINTLN("[LAYER_STACK]: Terminate")
    }

    void LayerStack::PushLayer(std::shared_ptr<Layer> layer)
    {
        _Layers.push_back(layer);
        layer->OnInit();
    }

    void LayerStack::Flush()
    {
        for (auto &layer : _Layers)
            layer->OnTerminate();
        _Layers.clear();
    }

} // namespace VEngine
