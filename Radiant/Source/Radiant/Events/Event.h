#pragma once

#include "Radiant/rdpch.h"

namespace Radiant {

	// Events are QUEUED (RAD-26): platform callbacks translate OS events and
	// push them into the application's EventQueue; handlers run when the queue
	// is processed at a single defined point at frame start (GameApplication::
	// Run), on the engine's own call stack — never inside OS callbacks.

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
	 * Base of the closed event hierarchy. Events are constructed by the
	 * platform layer and stored by value in the application's EventQueue;
	 * handlers receive them by reference — valid only for the duration of
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

	protected:
		// Copy/move are protected: derived events copy freely (EventQueue stores
		// them by value), while copying through an Event& — which would slice off
		// the derived half — does not compile. The user-declared destructor above
		// deprecates the implicit copies and suppresses the moves, so all five are
		// restated; declaring a constructor also suppresses the implicit default
		// constructor, hence Event() reappears here.
		Event() = default;
		Event(const Event&) = default;
		Event& operator=(const Event&) = default;
		Event(Event&&) = default;
		Event& operator=(Event&&) = default;
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
