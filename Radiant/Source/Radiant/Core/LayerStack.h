#pragma once

#include "Layer.h"

namespace Radiant {

	/**
	 * Ordered container of application layers — OWNS them: a pushed Layer* is
	 * deleted by the stack when popped (via ProcessPendingLayers) or when the
	 * stack is cleared/destroyed. One vector, partitioned: normal layers at the
	 * front, overlays at the back, so overlays update last (draw on top) and
	 * receive events first. All structural mutation is deferred — push/pop only
	 * queue, and ProcessPendingLayers() applies the changes between frames.
	 */
	class LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		/** Queues insertion into the front (layer) partition. The stack takes ownership immediately, even before insertion is applied. */
		void PushLayer(Layer* layer);
		/** Queues insertion into the back (overlay) partition. Same immediate ownership transfer as PushLayer. */
		void PushOverlay(Layer* overlay);
		/**
		 * Queues removal; OnDetach and deletion happen inside
		 * ProcessPendingLayers(). No-op if the layer is not currently in the
		 * stack — a layer pushed and popped within the same frame is NOT
		 * removed, because its insertion is still pending.
		 */
		void PopLayer(Layer* layer);
		/** PopLayer for overlays — identical semantics. */
		void PopOverlay(Layer* overlay);

		/**
		 * Applies queued pushes and pops, firing OnDetach on (and deleting)
		 * each removed layer. Must only run between frames — GameApplication
		 * calls it once per frame after updates and event polling — never while
		 * the stack is being iterated.
		 */
		void ProcessPendingLayers();

		/**
		 * Detaches and deletes every layer (including pending ones). Called by
		 * the destructor; GameApplication calls it explicitly to control
		 * shutdown order.
		 */
		void Clear();

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }

	private:
		std::vector<Layer*> m_Layers;
		std::vector<Layer*> m_PendingLayersToAdd;
		std::vector<Layer*> m_PendingLayersToRemove;
		unsigned int m_LayerInsertIndex = 0; // boundary between the layer (front) and overlay (back) partitions
	};

}