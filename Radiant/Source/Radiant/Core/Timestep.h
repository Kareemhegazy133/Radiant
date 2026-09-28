#pragma once

namespace Radiant {

	/**
	 * A frame's delta time. Implicitly converts to float SECONDS — write
	 * `velocity * ts`; use GetMilliseconds only for display. The value is a
	 * variable per-frame delta until the fixed-timestep rework (RAD-25).
	 */
	class Timestep
	{
	public:
		Timestep(float time = 0.0f)
			: m_Time(time)
		{
		}

		operator float() const { return m_Time; }

		float GetSeconds() const { return m_Time; }
		float GetMilliseconds() const { return m_Time * 1000.0f; }

	private:
		float m_Time;
	};

}