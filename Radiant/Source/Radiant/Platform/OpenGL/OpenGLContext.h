#pragma once

#include "Radiant/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Radiant {

	/**
	 * GL context for a GLFW window: Init() makes the context current on the
	 * calling thread, loads GL via glad, and asserts OpenGL >= 4.5. Non-owning
	 * view of the GLFWwindow — the Window owns both the handle and this
	 * context.
	 */
	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
	private:
		GLFWwindow* m_WindowHandle;
	};

}