#include "Radiant/rdpch.h"
#include "LayerStack.h"

namespace Radiant {

	LayerStack::LayerStack()
	{

	}

	LayerStack::~LayerStack()
	{
		Clear();
	}

	void LayerStack::Clear()
	{
		// Pending additions were never attached to the stack but are owned by it
		for (Layer* layer : m_PendingLayersToAdd)
			delete layer;
		m_PendingLayersToAdd.clear();
		m_PendingLayersToRemove.clear();

		for (Layer* layer : m_Layers)
		{
			layer->OnDetach();
			delete layer;
		}
		m_Layers.clear();
		m_LayerInsertIndex = 0;
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
				// Detach at actual removal — a popped layer receives updates/events
				// until the end of the frame, so it must stay attached until here
				layer->OnDetach();
				delete layer;
			}
		}
		m_PendingLayersToRemove.clear();
	}

}