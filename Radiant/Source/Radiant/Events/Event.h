#pragma once

#include "Radiant/rdpch.h"

namespace Radiant {

	// Events are currently BLOCKING: handlers execute synchronously inside the
	// GLFW callbacks, which fire during frame-start event polling
	// (Window::PollEvents). Handlers therefore run inside OS callbacks — do not
	// assume mid-frame safety or re-entrancy safety. Phase 2 (RAD-26) replaces
	// this with a queue drained from that same frame-start point; the type
	// system and dispatcher below survive that change unchanged.

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		GameAppTick, GameAppUpdate, GameAppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

// Generates the static/virtual type pair that lets EventDispatcher match a
// runtime event against a compile-time type without RTTI
#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

// Category bitflags let handlers filter event families via IsInCategory
// without enumerating concrete types
#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

	/**
	 * Base of the closed event hierarchy. Events are stack-allocated by the
	 * platform layer and passed by reference — valid only for the duration of
	 * dispatch; handlers must not store pointers or references to them. Setting
	 * Handled stops propagation to the layers beneath the current one.
	 */
	class Event
	{
	public:
		virtual ~Event() = default;

		bool Handled = false;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
	};

	/**
	 * Type-switch helper over a single event, held by reference — use within
	 * the scope that owns the event. Dispatch<T>(fn) runs fn only if the
	 * wrapped event is a T, OR-ing fn's bool result into Handled. Its return
	 * value reports whether the TYPE MATCHED, not whether the event was
	 * handled.
	 */
	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event)
			: m_Event(event)
		{
		}

		// F will be deduced by the compiler
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

}
