#pragma once

#include "Radiant/Layer.h"

#include "Radiant/Events/ApplicationEvent.h"
#include "Radiant/Events/KeyEvent.h"
#include "Radiant/Events/MouseEvent.h"

namespace Radiant {

	class RADIANT_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void Begin();
		void End();

	private:
		float m_Time = 0.0f;
	};

}