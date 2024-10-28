#pragma once

#include "ImGui/ImGuiLayer.h"

namespace Radiant {

	class OpenGLImGuiLayer : public ImGuiLayer
	{
	public:
		OpenGLImGuiLayer() = default;
		OpenGLImGuiLayer(const FontConfiguration& config);
		virtual ~OpenGLImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Begin() override;
		virtual void End() override;

		virtual void OnImGuiRender() override;

	private:
		FontConfiguration m_FontConfig;
	};

}