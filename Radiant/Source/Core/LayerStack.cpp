#include "rdpch.h"
#include "LayerStack.h"

namespace Radiant {

	LayerStack::LayerStack()
	{

	}

	LayerStack::~LayerStack()
	{

	}

	void LayerStack::PushLayer(Layer* layer)
	{
		layer->m_Type = LayerType::Normal;
		m_PendingLayersToAdd.emplace_back(layer);
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		overlay->m_Type = LayerType::Overlay;
		m_PendingLayersToAdd.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			m_PendingLayersToRemove.emplace_back(*it);
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
			m_PendingLayersToRemove.emplace_back(*it);
	}

	void LayerStack::ProcessPendingLayers()
	{
		// Add pending layers
		for (Layer* layer : m_PendingLayersToAdd)
		{
			if (layer->GetType() == LayerType::Normal)
			{
				m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
				m_LayerInsertIndex++;
			}
			else
			{
				m_Layers.emplace_back(layer);
			}
		}

		m_PendingLayersToAdd.clear();

		// Remove pending layers
		for (Layer* layer : m_PendingLayersToRemove)
		{
			auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
			if (it != m_Layers.end())
			{
				m_Layers.erase(it);
				if (layer->GetType() == LayerType::Normal)
				{
					m_LayerInsertIndex--;
				}
				delete layer;
			}
		}
		m_PendingLayersToRemove.clear();
	}

}