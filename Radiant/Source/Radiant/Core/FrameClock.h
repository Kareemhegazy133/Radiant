#pragma once

#include <cstdint>

namespace Radiant {

	/**
	 * Converts real frame time into whole fixed simulation steps plus an
	 * interpolation alpha — the engine's single owner of time. A value member
	 * of GameApplication; main-thread-only. All values are SECONDS, stored as
	 * double (float accumulators visibly drift within hours of runtime).
	 *
	 * Per frame: BeginFrame(realDelta) deposits scaled time into the
	 * accumulator, then `while (ConsumeStep())` runs one fixed simulation
	 * step per true. Time dilation scales the deposit — never the step size —
	 * so the fixed delta physics sees is constant at any time scale. Pause is
	 * time scale 0: nothing accumulates, no steps run.
	 */
	class FrameClock
	{
	public:
		/** Rate is fixed steps per simulation second (fixed delta = 1/rate). Zero is a programmer error (asserted). */
		explicit FrameClock(uint32_t simulationRate = 60);

		/**
		 * Deposits one frame of real time: scales it by the time scale, then
		 * clamps the accumulator to the max accumulation — the spiral-of-death
		 * valve; a hitch slows simulation instead of demanding unbounded
		 * catch-up steps. Clamping logs a WARN, throttled to once per second.
		 */
		void BeginFrame(double realFrameDelta);

		/**
		 * Consumes one fixed step from the accumulator if a full step is
		 * banked, advancing simulation time by exactly the fixed delta.
		 * Drives the fixed-update drain: `while (clock.ConsumeStep()) ...`.
		 */
		bool ConsumeStep();

		/**
		 * Fraction of the next step currently banked, in [0, 1) — how far the
		 * render frame sits between the last two simulation states. Render
		 * reads it for interpolation; simulation never does. Returns 1.0
		 * while paused, so rendering draws the settled current state rather
		 * than a frozen mid-blend pose.
		 */
		float GetAlpha() const;

		/**
		 * 1 = real time, 0.5 = slow motion, 0 = pause. A negative scale is a
		 * configuration mistake, not a programmer error: clamped to 0 with a
		 * WARN, never asserted.
		 */
		void SetTimeScale(float timeScale);
		float GetTimeScale() const { return (float)m_TimeScale; }

		double GetFixedDeltaTime() const { return m_FixedDeltaTime; }
		/** Simulation seconds elapsed: exactly steps-consumed × fixed delta, at any time scale. */
		double GetSimulationTime() const { return m_SimulationTime; }
		/** Real (unscaled) seconds deposited — log against GetSimulationTime to see dilation working. */
		double GetRealTime() const { return m_RealTime; }

	private:
		double m_FixedDeltaTime = 0.0;
		double m_Accumulator = 0.0;
		double m_TimeScale = 1.0;
		double m_SimulationTime = 0.0;
		double m_RealTime = 0.0;
		// Accumulator cap in seconds (Fiedler's clamp) — bounds catch-up steps after a hitch
		double m_MaxAccumulation = 0.25;
		// Throttles the can't-keep-up WARN to once per real second; a member (not a
		// static local) so independent clocks — e.g. in tests — don't share it
		double m_LastClampWarnTime = -1.0;
	};
}