#pragma once

#include "Layer.h"

namespace Radiant {

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

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }

	private:
		std::vector<Layer*> m_Layers;
		std::vector<Layer*> m_PendingLayersToAdd;
		std::vector<Layer*> m_PendingLayersToRemove;
		unsigned int m_LayerInsertIndex = 0;
	};

}