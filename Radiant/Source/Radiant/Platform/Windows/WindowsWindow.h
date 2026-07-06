#pragma once

#include <GLFW/glfw3.h>

#include "Radiant/Core/Window.h"
#include "Radiant/Renderer/GraphicsContext.h"

namespace Radiant {

	/**
	 * GLFW-backed Window implementation — the engine's only one. Owns the
	 * GLFWwindow handle and the GraphicsContext (Scope): the context dies with
	 * the window, so every GPU resource must be released before this destructor
	 * runs.
	 *
	 * GLFW callbacks translate OS events into Radiant events and dispatch them
	 * synchronously (blocking) through the callback set via SetEventCallback —
	 * they fire from inside PollEvents()'s glfwPollEvents, i.e. at frame start.
	 * The initial VSync state comes from the WindowSpecification; SetVSync maps
	 * it onto the GL swap interval, so it requires the context to be
	 * initialized. Main-thread only, like GLFW itself.
	 */
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowSpecification& specification);
		virtual ~WindowsWindow();

		void PollEvents() override;
		void Present() override;

		inline unsigned int GetWidth() const override { return m_Data.Width; }
		inline unsigned int GetHeight() const override { return m_Data.Height; }

		// Window attributes
		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		inline virtual void* GetNativeWindow() const override { return m_Window; }

	private:
		virtual void Init(const WindowSpecification& specification);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		WindowSpecification m_Specification;
		Scope<GraphicsContext> m_Context;

		// Handed to GLFW via the window user pointer so the C callbacks can
		// reach it without capturing `this`. Holds the live size — updated by
		// the resize callback; m_Specification keeps only the initial values.
		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}