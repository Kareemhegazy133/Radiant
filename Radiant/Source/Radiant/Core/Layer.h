#pragma once

#include "Radiant/Core/Timestep.h"
#include "Radiant/Events/Event.h"

namespace Radiant {

	// Which stack partition a layer belongs to — assigned by LayerStack on push,
	// never set manually
	enum class LayerType
	{
		Normal,
		Overlay
	};

	/**
	 * A coarse slice of the application (game view, UI overlay, future editor
	 * shell) — not a home for individual game systems; those live inside the
	 * Level. Heap-allocate and push into GameApplication; the layer stack owns
	 * the layer and deletes it at pop or shutdown. Hook contract: OnAttach
	 * fires at push, OnDetach at actual removal (end of the frame the pop was
	 * requested); OnUpdate runs every frame bottom→top with the frame delta in
	 * seconds; OnEvent runs top→bottom — set event.Handled to stop propagation.
	 * Event hooks currently execute inside OS callbacks at end of frame (see
	 * Events/Event.h).
	 */
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_DebugName; }
		inline const LayerType GetType() const { return m_Type; }

	protected:
		std::string m_DebugName;
		LayerType m_Type = LayerType::Normal;

		friend class LayerStack;
	};

}