#pragma once

#include "ImGui/ImGuiLayer.h"

namespace Radiant {

	class OpenGLImGuiLayer : public ImGuiLayer
	{
	public:
		OpenGLImGuiLayer();
		OpenGLImGuiLayer(const std::string& name);
		virtual ~OpenGLImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Begin() override;
		virtual void End() override;

		virtual void OnImGuiRender() override;

	private:
		float m_Time = 0.0f;
	};

}