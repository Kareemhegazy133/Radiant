#include "rdpch.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "OpenGLContext.h"

namespace Radiant {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		RADIANT_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		RADIANT_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		RADIANT_ASSERT(status, "Failed to initialize Glad!");

		RADIANT_INFO("OpenGL Info:");
		RADIANT_INFO("Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		RADIANT_INFO("Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		RADIANT_INFO("Version: {0}", (const char*)glGetString(GL_VERSION));

		RADIANT_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Radiant requires at least OpenGL version 4.5!");
	}

	void OpenGLContext::SwapBuffers()
	{
		RADIANT_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}