#pragma once

#include "Radiant/Core/Layer.h"

namespace Radiant {

	/**
	 * Renderer-backend-agnostic ImGui overlay. GameApplication calls Begin()/
	 * End() around the layer stack's OnImGuiRender pass each frame, so any layer
	 * can submit ImGui widgets between them without touching backend state.
	 */
	class ImGuiLayer : public Layer
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;

		/**
		 * Factory: returns the backend implementation matching the active
		 * RendererAPI (OpenGL today — the same switch pattern as the other
		 * renderer factories). Returns a raw pointer whose ownership transfers to
		 * the LayerStack via PushOverlay; the stack deletes it on Clear. Asserts
		 * and returns nullptr for an unknown or None API.
		 */
		static ImGuiLayer* Create();
	};
}