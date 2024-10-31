#pragma once

#include "Core/Timestep.h"
#include "Events/Event.h"

namespace Radiant {

	enum class LayerType
	{
		Normal,
		Overlay
	};

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