#include "Radiant/rdpch.h"
#include "FrameClock.h"

#include "Radiant/Core/Assert.h"

namespace Radiant {

	FrameClock::FrameClock(uint32_t simulationRate)
	{
		RADIANT_ASSERT(simulationRate > 0, "Simulation rate must be positive");
		m_FixedDeltaTime = 1.0 / simulationRate;
	}

	void FrameClock::BeginFrame(double realFrameDelta)
	{
		m_RealTime += realFrameDelta;
		m_Accumulator += realFrameDelta * m_TimeScale;

		if (m_Accumulator > m_MaxAccumulation)
		{
			// Spiral-of-death valve: drop the excess so a hitch slows the
			// simulation instead of demanding unbounded catch-up steps
			double dropped = m_Accumulator - m_MaxAccumulation;
			m_Accumulator = m_MaxAccumulation;

			if (m_RealTime - m_LastClampWarnTime >= 1.0)
			{
				RADIANT_WARN("FrameClock: simulation can't keep up - dropped {:.1f} ms of accumulated time", dropped * 1000.0);
				m_LastClampWarnTime = m_RealTime;
			}
		}
	}

	bool FrameClock::ConsumeStep()
	{
		if (m_Accumulator < m_FixedDeltaTime)
			return false;

		m_Accumulator -= m_FixedDeltaTime;
		m_SimulationTime += m_FixedDeltaTime;
		return true;
	}

	float FrameClock::GetAlpha() const
	{
		// Paused: render the settled current state, not a frozen mid-blend pose
		if (m_TimeScale == 0.0)
			return 1.0f;

		return (float)(m_Accumulator / m_FixedDeltaTime);
	}

	void FrameClock::SetTimeScale(float timeScale)
	{
		if (timeScale < 0.0f)
		{
			RADIANT_WARN("FrameClock: negative time scale ({}) clamped to 0 (pause)", timeScale);
			timeScale = 0.0f;
		}

		m_TimeScale = timeScale;
	}
}