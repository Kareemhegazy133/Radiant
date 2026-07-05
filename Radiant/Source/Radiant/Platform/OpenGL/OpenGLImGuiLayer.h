#pragma once

#include "Radiant/ImGui/ImGuiLayer.h"

namespace Radiant {

	/**
	 * ImGui layer wired to the GLFW + OpenGL3 backends. Owns the ImGui context
	 * lifecycle: context created in the constructor, backends initialized in
	 * OnAttach, everything torn down in OnDetach.
	 */
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