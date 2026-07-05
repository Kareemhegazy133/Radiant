#pragma once

#include "Radiant/ImGui/ImGuiLayer.h"

namespace Radiant {

	class OpenGLImGuiLayer : public ImGuiLayer
	{
	public:
		OpenGLImGuiLayer();
		virtual ~OpenGLImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Begin() override;
		virtual void End() override;

		virtual void OnImGuiRender() override;

	};

}