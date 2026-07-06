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
	 * requested). OnFixedUpdate runs 0..N times per frame bottom→top with the
	 * FIXED simulation delta — all simulation mutations belong here.
	 * OnUpdate runs exactly once per frame bottom→top with the real frame
	 * delta — render-rate work only; it must never mutate simulation state
	 * (playbook §1). OnEvent runs top→bottom — set event.Handled to stop
	 * propagation; event hooks currently execute inside OS callbacks at frame
	 * start (see Events/Event.h).
	 */
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnFixedUpdate(Timestep ts) {}
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