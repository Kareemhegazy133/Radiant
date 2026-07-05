#pragma once

#include <chrono>

namespace Radiant {

	/**
	 * Wall-clock stopwatch, started at construction. Elapsed() returns seconds,
	 * ElapsedMillis() milliseconds. For instrumentation and load timing — the
	 * frame timestep comes from GameApplication's loop, not from this.
	 */
	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		void Timer::Reset()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		float Timer::Elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
		}

		float Timer::ElapsedMillis()
		{
			return Elapsed() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

}