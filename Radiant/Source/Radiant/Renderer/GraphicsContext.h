#pragma once

#include "Radiant/Core/Base.h"

namespace Radiant {

	// Uniquely owned by the Window (Scope) — one ownership story per type
	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		static Scope<GraphicsContext> Create(void* window);
	};

}