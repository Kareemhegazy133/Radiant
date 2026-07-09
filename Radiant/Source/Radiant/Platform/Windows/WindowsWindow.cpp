#include "Radiant/rdpch.h"
#include "WindowsWindow.h"

#include <stb_image.h>

#include "Radiant/Events/EventQueue.h"

#include "Radiant/Core/Assert.h"

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
            if (!success)
            {
                // CRITICAL survives Dist (asserts don't): a shipped build that can't
                // boot GLFW must say so in the log before the inevitable crash
                RADIANT_CRITICAL("Could not initialize GLFW!");
                RADIANT_ASSERT(false, "Could not initialize GLFW!");
            }
            glfwSetErrorCallback(GLFWErrorCallback);
            s_GLFWInitialized = true;
        }

        {
			RADIANT_PROFILE_SCOPE("glfwCreateWindow");
			m_Window = glfwCreateWindow((int)specification.Width, (int)specification.Height, m_Data.Title.c_str(), nullptr, nullptr);
		}

		if (!m_Window)
		{
			RADIANT_CRITICAL("Failed to create window '{}' ({}x{}) - no GL 4.5 capable context?", m_Data.Title, m_Data.Width, m_Data.Height);
			RADIANT_ASSERT(false, "Window creation failed");
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
					// Specified-but-unloadable icon is a content mistake: say so
					RADIANT_WARN("Failed to load window icon '{}'", iconPathStr);
					useIcon = false;
				}
			}
		}

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(specification.VSync);

		// Set GLFW callbacks. Enqueue ONLY — no engine handler runs inside an
		// OS callback; GameApplication processes the queue at frame start
		// (RAD-26). A null Queue is the boot edge between window creation and
		// SetEventQueue wiring: those events are dropped by design.
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				// The live-size mirror stays immediate (state, not dispatch) —
				// GetWidth() is current even before the resize event processes
				data.Width = width;
				data.Height = height;

				if (data.Queue)
					data.Queue->Push(WindowResizeEvent(width, height));
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				if (data.Queue)
					data.Queue->Push(WindowCloseEvent());
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				if (!data.Queue)
					return;

				switch (action)
				{
				case GLFW_PRESS:
					data.Queue->Push(KeyPressedEvent(key, false));
					break;
				case GLFW_RELEASE:
					data.Queue->Push(KeyReleasedEvent(key));
					break;
				case GLFW_REPEAT:
					data.Queue->Push(KeyPressedEvent(key, true));
					break;
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				if (!data.Queue)
					return;

				switch (action)
				{
				case GLFW_PRESS:
					data.Queue->Push(MouseButtonPressedEvent(button));
					break;
				case GLFW_RELEASE:
					data.Queue->Push(MouseButtonReleasedEvent(button));
					break;
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				if (data.Queue)
					data.Queue->Push(MouseScrolledEvent((float)xOffset, (float)yOffset));
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				if (data.Queue)
					data.Queue->Push(MouseMovedEvent((float)xPos, (float)yPos));
			});
    }

	void WindowsWindow::SetEventQueue(EventQueue* queue)
	{
		RADIANT_ASSERT(queue, "SetEventQueue: null queue");
		m_Data.Queue = queue;
	}

    void WindowsWindow::Shutdown()
	{
		RADIANT_PROFILE_FUNCTION();

        glfwDestroyWindow(m_Window);
		// Single-window assumption: terminating GLFW here tears down every
		// window and context in the process
		glfwTerminate();
    }

    void WindowsWindow::PollEvents()
	{
		RADIANT_PROFILE_FUNCTION();

        glfwPollEvents();
    }

	void WindowsWindow::Present()
	{
		RADIANT_PROFILE_FUNCTION();

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
