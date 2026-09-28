#pragma once

#include "Radiant/Core/Base.h"

namespace Radiant {

	/**
	 * Graphics-API context bound to a native window. Uniquely owned by the
	 * Window (Scope) — one ownership story per type — so the context dies with
	 * its window; every GPU resource must be released before that happens.
	 * Main-thread only.
	 *
	 * Init() must run before any other renderer call. Create() takes the native
	 * window handle (a GLFWwindow* today) as an opaque pointer so this header
	 * stays free of platform types.
	 */
	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		/** Presents the completed frame to the window. */
		virtual void SwapBuffers() = 0;

		static Scope<GraphicsContext> Create(void* window);
	};

}