#include "Radiant/rdpch.h"
#include "EventQueue.h"

#include "Radiant/Core/Assert.h"

namespace Radiant {

	// Covers realistic frames without growth: a 1 kHz mouse at 60 FPS delivers
	// ~17 events per frame. Floods grow the buffers once; capacity then
	// persists for the life of the app (clear() never releases it).
	static constexpr size_t s_InitialEventCapacity = 64;

	EventQueue::EventQueue()
	{
		m_PendingEvents.reserve(s_InitialEventCapacity);
		m_ProcessingEvents.reserve(s_InitialEventCapacity);
	}

	void EventQueue::ProcessEvents(const EventSink& sink)
	{
		RADIANT_ASSERT(!m_IsProcessingEvents, "ProcessEvents re-entered");
		RADIANT_ASSERT(sink, "ProcessEvents: null sink");

		m_IsProcessingEvents = true;

		// Swap, never iterate the pending buffer in place: a handler that
		// pushes would grow the vector under the loop and invalidate it.
		// Mid-processing pushes land in the (now empty) pending buffer and
		// are processed next frame.
		std::swap(m_PendingEvents, m_ProcessingEvents);

		for (QueuedEvent& queuedEvent : m_ProcessingEvents)
		{
			// auto& deduces the concrete event type; the sink receives it as
			// Event& (a reference upcast), so handlers mutate the real object
			std::visit([&sink](auto& event) { sink(event); }, queuedEvent);
		}

		// clear() keeps capacity — the steady state allocates nothing
		m_ProcessingEvents.clear();

		m_IsProcessingEvents = false;
	}
}