#pragma once

#include "Radiant/Events/Event.h"
#include "Radiant/Events/ApplicationEvent.h"
#include "Radiant/Events/KeyEvent.h"
#include "Radiant/Events/MouseEvent.h"

#include <type_traits>
#include <variant>

namespace Radiant {

	/**
	 * Buffers translated OS events between platform delivery and the
	 * frame-start processing point, preserving arrival order. Owned by value
	 * by GameApplication and declared before the window there, so the window's
	 * non-owning pointer can never dangle. Main-thread-only — GLFW invokes its
	 * callbacks on the thread that calls glfwPollEvents. Events pushed while
	 * ProcessEvents is running are processed next frame, deterministically.
	 */
	class EventQueue
	{
	public:
		/** Receives each processed event by reference, exactly once, in arrival order. */
		using EventSink = std::function<void(Event&)>;

		/**
		 * One alternative per concrete event type. Adding an event type is one
		 * line here — Push of an unlisted type fails to compile.
		 */
		using QueuedEvent = std::variant<
			WindowResizeEvent, WindowCloseEvent,
			KeyPressedEvent, KeyReleasedEvent, KeyTypedEvent,
			MouseButtonPressedEvent, MouseButtonReleasedEvent,
			MouseMovedEvent, MouseScrolledEvent>;

		// A fat payload (a std::string, a path list) in any one event bloats
		// EVERY slot in the queue — big data travels as a handle to side
		// storage, never by value
		static_assert(sizeof(QueuedEvent) <= 64, "Queued events must stay compact");

		EventQueue();

		// The window holds a pointer to ONE specific instance — a copy would be
		// a meaningless queue receiving no events, so copying does not compile
		EventQueue(const EventQueue&) = delete;
		EventQueue& operator=(const EventQueue&) = delete;

		/**
		 * Enqueues a copy (or moves a temporary) of a concrete event for the
		 * next ProcessEvents call. Never runs handlers itself, so it is safe
		 * to call from OS callbacks at any point — including while
		 * ProcessEvents is running (the event is processed next frame).
		 */
		template<typename T>
		void Push(T&& event)
		{
			static_assert(std::is_constructible_v<QueuedEvent, T&&>,
				"Event type is not in the QueuedEvent variant list (EventQueue.h) - add it there");
			m_PendingEvents.emplace_back(std::forward<T>(event));
		}

		/**
		 * Hands every pending event to the sink, in arrival order, then clears
		 * the queue. The single frame-start processing point: called once per
		 * frame by GameApplication::Run between PollEvents and simulation.
		 * Not re-entrant (asserted) — handlers must not call back into it.
		 */
		void ProcessEvents(const EventSink& sink);

	private:
		std::vector<QueuedEvent> m_PendingEvents;    // arrivals land here
		std::vector<QueuedEvent> m_ProcessingEvents; // walked by ProcessEvents
		bool m_IsProcessingEvents = false;           // re-entrancy tripwire
	};
}