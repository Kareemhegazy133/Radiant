#include "rdpch.h"
#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <gl/GL.h>

namespace Radiant {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle)
	{
		RD_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		RD_CORE_ASSERT(status, "Failed to initialize Glad!");

		RD_CORE_INFO("OpenGL Info:");
		RD_CORE_INFO(" Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		RD_CORE_INFO(" Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		RD_CORE_INFO(" Version: {0}", (const char*)glGetString(GL_VERSION));
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}
}