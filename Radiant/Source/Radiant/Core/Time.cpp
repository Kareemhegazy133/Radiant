#include "Radiant/rdpch.h"
#include "Time.h"

#include "GameApplication.h"

namespace Radiant {

	void Time::SetTimeScale(float timeScale)
	{
		GameApplication::Get().m_Clock.SetTimeScale(timeScale);
	}

	float Time::GetTimeScale()
	{
		return GameApplication::Get().m_Clock.GetTimeScale();
	}

	float Time::GetAlpha()
	{
		return GameApplication::Get().m_Clock.GetAlpha();
	}

	double Time::GetFixedDeltaTime()
	{
		return GameApplication::Get().m_Clock.GetFixedDeltaTime();
	}

	double Time::GetSimulationTime()
	{
		return GameApplication::Get().m_Clock.GetSimulationTime();
	}

	double Time::GetRealTime()
	{
		return GameApplication::Get().m_Clock.GetRealTime();
	}

}