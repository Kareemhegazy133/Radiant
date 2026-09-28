#pragma once

namespace Radiant {

	/**
	 * Miscellaneous platform utilities. The date-time functions use the
	 * machine's LOCAL time at minute precision.
	 */
	class Platform
	{
	public:
		/** Local date-time packed into decimal digits (YYYYMMDDHHMM) — numeric order matches chronological order. */
		static uint64_t GetCurrentDateTimeU64();
		/** Local date-time as "YYYYMMDDHHMM" (minute precision). */
		static std::string GetCurrentDateTimeString();

	};

}