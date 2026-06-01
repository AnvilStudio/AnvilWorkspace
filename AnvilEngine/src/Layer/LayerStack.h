#pragma once
#include "Layer.h"
#include <vector>

namespace anv
{
    class LayerStack
    {
    public:
        ~LayerStack()
        {
            for (Layer* layer : m_Layers)
                delete layer;
        }

        void PushLayer(Layer* layer)
        {
            m_Layers.emplace(
                m_Layers.begin() + m_LayerInsertIndex,
                layer
            );

            m_LayerInsertIndex++;
            layer->OnAttach();
        }

        void PushOverlay(Layer* overlay)
        {
            m_Layers.emplace_back(overlay);
            overlay->OnAttach();
        }

        void PopLayer(Layer* layer)
        {
            auto it = std::find(
                m_Layers.begin(),
                m_Layers.begin() + m_LayerInsertIndex,
                layer
            );

            if (it != m_Layers.begin() + m_LayerInsertIndex)
            {
                layer->OnDetach();
                delete layer;
                m_Layers.erase(it);
                m_LayerInsertIndex--;
            }
        }

        void PopOverlay(Layer* overlay)
        {
            auto it = std::find(
                m_Layers.begin() + m_LayerInsertIndex,
                m_Layers.end(),
                overlay
            );

            if (it != m_Layers.end())
            {
                overlay->OnDetach();
                delete overlay;
                m_Layers.erase(it);
            }
        }

        auto begin() { return m_Layers.begin(); }
        auto end() { return m_Layers.end(); }

    private:
        std::vector<Layer*> m_Layers;
        uint32_t m_LayerInsertIndex = 0;
    };
}