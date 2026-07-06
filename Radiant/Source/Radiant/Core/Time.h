#pragma once

namespace Radiant {

	/**
	 * Stateless static facade over the application's FrameClock — the engine's
	 * public time API. Valid only while the GameApplication exists (calls
	 * before construction assert). Deliberately excludes the loop-driving
	 * methods (BeginFrame/ConsumeStep): game code observes time and may scale
	 * it, never advance it. Header stays include-free so gameplay code can ask
	 * the time without paying for the application's headers.
	 *
	 * Vocabulary — none of these mean frame rate (frames per second is a
	 * rendering fact the simulation no longer depends on):
	 *  - fixed delta: the constant size of one simulation step (1/SimulationRate)
	 *  - simulation time: seconds the game world has lived — exactly
	 *    steps × fixed delta; freezes under pause, halves under 0.5 dilation
	 *  - real time: unscaled seconds deposited by the loop — tracks the wall
	 *    clock while running, freezes while minimized (nothing is deposited)
	 */
	class Time
	{
	public:
		/** 1 = real time, 0.5 = slow motion, 0 = pause (render/UI unaffected). Negative clamps to 0 with a WARN. */
		static void SetTimeScale(float timeScale);
		static float GetTimeScale();

		/**
		 * Fraction of the next simulation step banked, in [0, 1) — how far the
		 * current render frame sits between the last two simulation states.
		 * For render interpolation only; 1.0 while paused.
		 */
		static float GetAlpha();

		static double GetFixedDeltaTime();
		static double GetSimulationTime();
		static double GetRealTime();
	};

}