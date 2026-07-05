#pragma once

#include "Radiant/Events/Event.h"

namespace Radiant {

	/**
	 * Window client-area resize. Width/height are the OS-reported window size
	 * in screen coordinates (not framebuffer pixels); 0x0 signals minimization
	 * (GameApplication suspends updates on it).
	 */
	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		unsigned int GetWidth() const { return m_Width; }
		unsigned int GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		unsigned int m_Width, m_Height;
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	// The GameAppTick/Update/Render events below are defined but never emitted
	// by the engine (Hazel leftovers) — do not rely on receiving them
	class GameAppTickEvent : public Event
	{
	public:
		GameAppTickEvent() = default;

		EVENT_CLASS_TYPE(GameAppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class GameAppUpdateEvent : public Event
	{
	public:
		GameAppUpdateEvent() = default;

		EVENT_CLASS_TYPE(GameAppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class GameAppRenderEvent : public Event
	{
	public:
		GameAppRenderEvent() = default;

		EVENT_CLASS_TYPE(GameAppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};
}