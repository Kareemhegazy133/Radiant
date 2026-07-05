#include "Radiant/rdpch.h"
#include "WindowsWindow.h"

#include <stb_image.h>

#include "Radiant/Events/ApplicationEvent.h"
#include "Radiant/Events/MouseEvent.h"
#include "Radiant/Events/KeyEvent.h"

#include "Radiant/Platform/OpenGL/OpenGLContext.h"

namespace Radiant {

    static bool s_GLFWInitialized = false;

    static void GLFWErrorCallback(int error, const char* description)
    {
        RADIANT_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    WindowsWindow::WindowsWindow(const WindowSpecification& specification)
		: m_Specification(specification)
	{
		RADIANT_PROFILE_FUNCTION();

        Init(specification);
    }

    WindowsWindow::~WindowsWindow()
	{
		RADIANT_PROFILE_FUNCTION();

        Shutdown();
    }

    void WindowsWindow::Init(const WindowSpecification& specification)
	{
		RADIANT_PROFILE_FUNCTION();

        m_Data.Title = specification.Title;
        m_Data.Width = specification.Width;
        m_Data.Height = specification.Height;

        RADIANT_INFO("Creating window {0} ({1}, {2})", specification.Title, specification.Width, specification.Height);

        if (!s_GLFWInitialized)
        {
            int success = glfwInit();
            RADIANT_ASSERT(success, "Could not intialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
            s_GLFWInitialized = true;
        }

        {
			RADIANT_PROFILE_SCOPE("glfwCreateWindow");
			m_Window = glfwCreateWindow((int)specification.Width, (int)specification.Height, m_Data.Title.c_str(), nullptr, nullptr);
		}

		// Set icon
		{
			GLFWimage icon;
			int channels;

			bool useIcon = !m_Specification.IconPath.empty();

			if (useIcon)
			{
				std::string iconPathStr = m_Specification.IconPath.string();
				icon.pixels = stbi_load(iconPathStr.c_str(), &icon.width, &icon.height, &channels, 4);
				if (icon.pixels)
				{
					glfwSetWindowIcon(m_Window, 1, &icon);
					stbi_image_free(icon.pixels);
				}
				else
				{
					useIcon = false;
				}
			}
		}

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(specification.VSync);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});
    }

    void WindowsWindow::Shutdown()
	{
		RADIANT_PROFILE_FUNCTION();

        glfwDestroyWindow(m_Window);
		glfwTerminate();
    }

    void WindowsWindow::OnUpdate()
	{
		RADIANT_PROFILE_FUNCTION();

        glfwPollEvents();
		m_Context->SwapBuffers();
    }

    void WindowsWindow::SetVSync(bool enabled)
	{
		RADIANT_PROFILE_FUNCTION();

		enabled ? glfwSwapInterval(1) : glfwSwapInterval(0);

        m_Specification.VSync = enabled;
    }

    bool WindowsWindow::IsVSync() const
    {
        return m_Specification.VSync;
    }

}
