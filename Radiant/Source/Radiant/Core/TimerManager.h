#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace Radiant {

	/**
	 * Opaque, copyable identity for a scheduled timer. Index says which slot,
	 * Generation says which lifetime of that slot — so a handle to a dead
	 * timer (even one whose slot was reused) is always safe: ClearTimer on it
	 * is a no-op and IsActive returns false. Value type; hold it as long as
	 * you like.
	 */
	struct TimerHandle
	{
		static constexpr uint32_t InvalidIndex = 0xFFFFFFFF;

		uint32_t Index = InvalidIndex;
		uint32_t Generation = 0;
	};

	/**
	 * Schedules callbacks N SIMULATION seconds out (one-shot or looping) —
	 * ticked once per fixed step with the fixed delta, so timers dilate with
	 * the time scale and freeze under pause with no special casing. A value
	 * member of GameApplication (per-Level migration planned with
	 * play-in-editor, RAD-52); main-thread-only; non-copyable (callbacks are
	 * identity).
	 *
	 * CALLBACK LIFETIME CONTRACT: the manager owns the callback by value. A
	 * callback capturing an object (Entity, Level*, this) outlives its target
	 * unless the owner clears the handle in its teardown path — ClearTimer
	 * releases the callback (and its captures) immediately.
	 *
	 * Internals mirror UE's FTimerManager: slot storage + free list with
	 * generation counters, and a min-heap of (expiry, handle) entries with
	 * lazy invalidation — cancelled timers leave stale heap entries that are
	 * discarded on pop via the generation check. A tick with nothing due
	 * costs one comparison. Tick allocates nothing itself, with one caveat:
	 * a looping fire invokes a COPY of its callback (re-entrancy safety), so
	 * captures that outgrow the std::function small buffer allocate per fire.
	 */
	class TimerManager
	{
	public:
		TimerManager() = default;
		// Non-copyable: copying would duplicate callback identity (playbook §2)
		TimerManager(const TimerManager&) = delete;
		TimerManager& operator=(const TimerManager&) = delete;

		/**
		 * Schedules callback delaySeconds of simulation time from now; looping
		 * timers re-arm at expiry + period (no drift — lateness never
		 * compounds). A negative delay is clamped to 0 with a WARN; a
		 * zero-delay looping request degrades to one-shot with a WARN (a zero
		 * period would re-fire forever within one tick); an empty callback is
		 * a programmer error (asserted, returns an invalid handle). See the
		 * class doc for the callback lifetime contract.
		 */
		TimerHandle SetTimer(float delaySeconds, std::function<void()> callback, bool looping = false);

		/**
		 * Cancels the timer and releases its callback (and captures)
		 * immediately; resets the handle. Stale, invalid, or already-fired
		 * handles are benign no-ops by design — this is the correct idiom for
		 * "clear if still pending".
		 */
		void ClearTimer(TimerHandle& handle);

		/** True while the timer is scheduled (a one-shot goes inactive just before its callback runs). */
		bool IsActive(TimerHandle handle) const;

		/**
		 * Advances the internal simulation clock and fires everything due, in
		 * expiry order. Called by GameApplication once per fixed step, before
		 * layer OnFixedUpdate. Callbacks may re-entrantly Set/Clear timers —
		 * the manager's state is consistent before any callback runs.
		 */
		void Tick(double fixedDelta);

	private:
		struct Timer
		{
			std::function<void()> Callback;
			double Expiry = 0.0;       // in simulation seconds against m_Time
			double Period = 0.0;       // > 0 = looping
			uint32_t Generation = 0;
			bool Active = false;
		};

		struct HeapEntry
		{
			double Expiry;
			TimerHandle Handle;
		};

		uint32_t AllocateSlot();

		std::vector<Timer> m_Timers;       // slot storage — never shrinks, slots recycle
		std::vector<uint32_t> m_FreeList;  // indices of released slots awaiting reuse
		std::vector<HeapEntry> m_Heap;     // min-heap by Expiry (std::push_heap/pop_heap)
		double m_Time = 0.0;               // simulation seconds; advances only in Tick
	};

}