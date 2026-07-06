#pragma once

#include "Radiant/rdpch.h"

#include "Base.h"
#include "Radiant/Events/Event.h"

namespace Radiant {

	struct WindowSpecification
	{
		std::string Title = "Game";
		uint32_t Width = 1280;
		uint32_t Height = 720;
		bool VSync = true;
		std::filesystem::path IconPath;
	};

	/**
	 * Abstract desktop window — the engine's OS seam. The platform
	 * implementation (WindowsWindow today) owns the native window and the
	 * graphics context. Create() returns it as an owning Scope, held by
	 * GameApplication for the life of the app.
	 */
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {}

		/**
		 * Pumps OS events. The OS event callbacks — and therefore all engine
		 * event handlers — execute synchronously inside this call (a queued
		 * dispatch replaces that in RAD-26). Invoked once per frame by
		 * GameApplication at frame START, so the simulation steps see this
		 * frame's input.
		 */
		virtual void PollEvents() = 0;

		/**
		 * Presents the rendered frame (buffer swap). Invoked once per frame by
		 * GameApplication at frame end — including while minimized, which the
		 * platform layer must tolerate.
		 */
		virtual void Present() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Window attributes
		/**
		 * Sets the sink that receives translated OS events (bound to
		 * GameApplication::OnEvent). Invoked synchronously from inside OS
		 * callbacks during PollEvents(); must remain valid for the lifetime of
		 * the window.
		 */
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		/** Non-owning backend handle (GLFWwindow* today). For platform-layer use — engine and game code should not need it. */
		virtual void* GetNativeWindow() const = 0;

		/**
		 * Factory for the platform implementation; the caller owns the returned
		 * window. Asserts and returns null on an unsupported platform.
		 */
		static Scope<Window> Create(const WindowSpecification& specification = WindowSpecification());
	};

}