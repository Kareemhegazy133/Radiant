#pragma once

#include "Radiant/rdpch.h"

#include "Base.h"

namespace Radiant {

	class EventQueue;

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
		virtual ~Window() {}

		/**
		 * Pumps OS events. The platform callbacks translate each OS event and
		 * push it into the queue set via SetEventQueue — no engine handlers
		 * execute inside this call. Invoked once per frame by GameApplication
		 * at frame START; the queue is processed immediately after it returns,
		 * so the simulation steps see this frame's input.
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
		 * Sets the queue that receives translated OS events. Non-owning: the
		 * queue must outlive the window (GameApplication guarantees this by
		 * member declaration order). Wire it before the first PollEvents — a
		 * null queue is a programmer error (asserted); events arriving before
		 * wiring are dropped.
		 */
		virtual void SetEventQueue(EventQueue* queue) = 0;
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