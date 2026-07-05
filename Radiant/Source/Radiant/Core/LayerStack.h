#pragma once

#include "Layer.h"

namespace Radiant {

	// Owns its layers: a pushed Layer* is deleted by the stack when popped
	// (via ProcessPendingLayers) or when the stack is cleared/destroyed.
	class LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		void ProcessPendingLayers();

		// Detaches and deletes every layer (including pending ones). Called by the
		// destructor; GameApplication calls it explicitly to control shutdown order.
		void Clear();

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }

	private:
		std::vector<Layer*> m_Layers;
		std::vector<Layer*> m_PendingLayersToAdd;
		std::vector<Layer*> m_PendingLayersToRemove;
		unsigned int m_LayerInsertIndex = 0;
	};

}