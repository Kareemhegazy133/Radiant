#include "Radiant/rdpch.h"
#include "TimerManager.h"

#include "Radiant/Core/Assert.h"

#include <algorithm>

namespace Radiant {

	// The heap comparators below invert std::push_heap/pop_heap's default
	// max-heap into soonest-expiry-first ("greater expiry sorts down")

	TimerHandle TimerManager::SetTimer(float delaySeconds, std::function<void()> callback, bool looping)
	{
		RADIANT_ASSERT(callback, "SetTimer: empty callback");
		if (!callback)
			return {};

		if (delaySeconds < 0.0f)
		{
			RADIANT_WARN("TimerManager: negative delay ({}) clamped to 0", delaySeconds);
			delaySeconds = 0.0f;
		}

		if (looping && delaySeconds == 0.0f)
		{
			// A zero-period looper would re-fire forever within a single Tick
			RADIANT_WARN("TimerManager: zero-delay looping timer not supported - scheduling as one-shot");
			looping = false;
		}

		uint32_t index = AllocateSlot();
		Timer& timer = m_Timers[index];
		timer.Callback = std::move(callback);
		timer.Expiry = m_Time + delaySeconds;
		timer.Period = looping ? (double)delaySeconds : 0.0;
		timer.Active = true;

		TimerHandle handle{ index, timer.Generation };
		m_Heap.push_back({ timer.Expiry, handle });
		std::push_heap(m_Heap.begin(), m_Heap.end(),
			[](const HeapEntry& a, const HeapEntry& b) { return a.Expiry > b.Expiry; });

		return handle;
	}

	void TimerManager::ClearTimer(TimerHandle& handle)
	{
		if (!IsActive(handle))
		{
			handle = {};
			return;
		}

		Timer& timer = m_Timers[handle.Index];
		timer.Active = false;
		timer.Generation++;              // stale heap entries now fail the check
		timer.Callback = nullptr;        // release captures NOW (lifetime contract)
		m_FreeList.push_back(handle.Index);
		handle = {};
	}

	bool TimerManager::IsActive(TimerHandle handle) const
	{
		return handle.Index < m_Timers.size()
			&& m_Timers[handle.Index].Active
			&& m_Timers[handle.Index].Generation == handle.Generation;
	}

	void TimerManager::Tick(double fixedDelta)
	{
		m_Time += fixedDelta;

		auto later = [](const HeapEntry& a, const HeapEntry& b) { return a.Expiry > b.Expiry; };

		while (!m_Heap.empty() && m_Heap.front().Expiry <= m_Time)
		{
			std::pop_heap(m_Heap.begin(), m_Heap.end(), later);
			HeapEntry entry = m_Heap.back();
			m_Heap.pop_back();

			// Lazy invalidation: cancelled/re-armed timers leave stale entries
			// behind — the generation check discards them here
			if (!IsActive(entry.Handle))
				continue;

			Timer& timer = m_Timers[entry.Handle.Index];

			if (timer.Period > 0.0)
			{
				// Re-arm at expiry + period (not now + period): lateness never
				// compounds, the cadence stays anchored to the original schedule
				timer.Expiry = entry.Expiry + timer.Period;
				m_Heap.push_back({ timer.Expiry, entry.Handle });
				std::push_heap(m_Heap.begin(), m_Heap.end(), later);

				// Invoke a COPY: the callback may SetTimer re-entrantly, which can
				// reallocate m_Timers and dangle any reference into it
				auto callback = timer.Callback;
				callback();
			}
			else
			{
				// Pop-before-invoke: retire the slot completely, then run user
				// code against a consistent manager (callbacks may Set/Clear)
				auto callback = std::move(timer.Callback);
				timer.Active = false;
				timer.Generation++;
				m_FreeList.push_back(entry.Handle.Index);

				callback();
			}
		}
	}

	uint32_t TimerManager::AllocateSlot()
	{
		if (!m_FreeList.empty())
		{
			uint32_t index = m_FreeList.back();
			m_FreeList.pop_back();
			return index;
		}

		m_Timers.emplace_back();
		return (uint32_t)(m_Timers.size() - 1);
	}

}