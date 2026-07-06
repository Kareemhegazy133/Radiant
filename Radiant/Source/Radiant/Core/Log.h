#pragma once

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include "Base.h"

// fmt::formatter specializations for engine/vendor types (UUID, path, glm) —
// included here so every log call site can format them
#include "LogCustomFormatters.h"

namespace Radiant {

	/**
	 * Logging bootstrap over spdlog: one engine logger ("RADIANT") and one game
	 * logger ("GAME"), both writing to stdout and Radiant.log (working-dir
	 * relative; falls back to console-only with a WARN if the file cannot be
	 * opened). Init() must run once before any log macro — the engine-owned
	 * main() calls it first; logging before Init dereferences a null logger.
	 * Use the RADIANT_* / GAME_* macros below rather than the loggers directly:
	 * TRACE/INFO compile out of Dist builds entirely (arguments unevaluated,
	 * format strings absent from the shipping binary); WARN and above survive
	 * in every config.
	 */
	class Log
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>& GetRadiantLogger() { return s_RadiantLogger; }
		static std::shared_ptr<spdlog::logger>& GetGameLogger() { return s_GameLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_RadiantLogger;
		static std::shared_ptr<spdlog::logger> s_GameLogger;
	};

}

// TRACE/INFO are development chatter: stripped from Dist at compile time so
// shipping builds neither evaluate their arguments nor carry their strings.
// WARN/ERROR/CRITICAL remain in all configs — a player's log of what went
// wrong is often the only diagnostic a shipped build produces.
#ifndef RD_DIST
	#define RADIANT_TRACE(...)    ::Radiant::Log::GetRadiantLogger()->trace(__VA_ARGS__)
	#define RADIANT_INFO(...)     ::Radiant::Log::GetRadiantLogger()->info(__VA_ARGS__)
	#define GAME_TRACE(...)       ::Radiant::Log::GetGameLogger()->trace(__VA_ARGS__)
	#define GAME_INFO(...)        ::Radiant::Log::GetGameLogger()->info(__VA_ARGS__)
#else
	#define RADIANT_TRACE(...)    ((void)0)
	#define RADIANT_INFO(...)     ((void)0)
	#define GAME_TRACE(...)       ((void)0)
	#define GAME_INFO(...)        ((void)0)
#endif

#define RADIANT_WARN(...)     ::Radiant::Log::GetRadiantLogger()->warn(__VA_ARGS__)
#define RADIANT_ERROR(...)    ::Radiant::Log::GetRadiantLogger()->error(__VA_ARGS__)
#define RADIANT_CRITICAL(...) ::Radiant::Log::GetRadiantLogger()->critical(__VA_ARGS__)

#define GAME_WARN(...)        ::Radiant::Log::GetGameLogger()->warn(__VA_ARGS__)
#define GAME_ERROR(...)       ::Radiant::Log::GetGameLogger()->error(__VA_ARGS__)
#define GAME_CRITICAL(...)    ::Radiant::Log::GetGameLogger()->critical(__VA_ARGS__)